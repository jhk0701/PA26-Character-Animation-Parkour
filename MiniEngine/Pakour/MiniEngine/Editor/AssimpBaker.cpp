#include "pch.h"
#include "Editor/AssimpBaker.h"

#if defined(WITH_EDITOR)
#include "Asset/MiniFormat.h"
#include "Asset/MiniLoader.h" // WriteStaticMesh/WriteSkinnedMesh (+Skeleton/AnimClip 타입)
#include "Asset/BoneNaming.h" // NormalizeBoneName/ResolveHumanoidBone (런타임과 공용)
#include "Core/Log.h"
#include "Core/Math.h"

#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include <assimp/config.h> // AI_CONFIG_IMPORT_FBX_PRESERVE_PIVOTS

#include <vector>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <set>       // 리타게팅 샘플 타임 합집합(정렬·중복제거)
#include <cstdint>
#include <cmath>
#include <cctype> // std::tolower (본 이름 정규화)
#include <algorithm> // std::max/min — 리그 정렬(MinimalRotation) 의 dot 클램프
#include <windows.h> // WideCharToMultiByte (wstring → UTF-8 경로)

namespace MiniEngine
{
    namespace Editor
    {
        namespace
        {
            std::string ToUtf8(const std::wstring& _w)
            {
                if (_w.empty()) return {};
                const int len = ::WideCharToMultiByte(CP_UTF8, 0, _w.c_str(),
                    static_cast<int>(_w.size()), nullptr, 0, nullptr, nullptr);
                std::string out(static_cast<size_t>(len), '\0');
                ::WideCharToMultiByte(CP_UTF8, 0, _w.c_str(), static_cast<int>(_w.size()),
                    out.data(), len, nullptr, nullptr);
                return out;
            }

            // 경로 → 파일 stem(디렉터리/확장자 제거, UTF-8) — 클립 이름 fallback 용.
            std::string FileStemUtf8(const std::wstring& _path)
            {
                std::wstring stem = _path;
                const size_t slash = stem.find_last_of(L"\\/");
                if (slash != std::wstring::npos) stem = stem.substr(slash + 1);
                const size_t dot = stem.find_last_of(L'.');
                if (dot != std::wstring::npos) stem = stem.substr(0, dot);
                return ToUtf8(stem);
            }

            // 공통 임포트(메인/추가 애니 소스 동일 플래그). importer 가 scene 을 소유하므로
            // 호출부가 importer 를 scene 수명 동안 유지해야 한다.
            const aiScene* ImportScene(Assimp::Importer& _importer, const std::wstring& _path)
            {
                // FBX pivot 헬퍼 노드($AssimpFbx$_PreRotation 등)를 본 노드로 병합 — 스켈레톤이
                // 실제 본 수의 몇 배로 부풀어 MAX_BONES(128)를 넘는 것을 방지(예: Mixamo 65→219).
                _importer.SetPropertyBool(AI_CONFIG_IMPORT_FBX_PRESERVE_PIVOTS, false);
                const unsigned int flags =
                    aiProcess_Triangulate |
                    aiProcess_GenSmoothNormals |
                    aiProcess_JoinIdenticalVertices |
                    aiProcess_LimitBoneWeights |
                    aiProcess_GlobalScale |
                    aiProcess_ConvertToLeftHanded;
                return _importer.ReadFile(ToUtf8(_path), flags);
            }

            // aiMatrix4x4(행 우선)로 점을 변환(w=1, translation 포함).
            void TransformPoint(const aiMatrix4x4& _m, const aiVector3D& _p, float _out[3])
            {
                _out[0] = _m.a1 * _p.x + _m.a2 * _p.y + _m.a3 * _p.z + _m.a4;
                _out[1] = _m.b1 * _p.x + _m.b2 * _p.y + _m.b3 * _p.z + _m.b4;
                _out[2] = _m.c1 * _p.x + _m.c2 * _p.y + _m.c3 * _p.z + _m.c4;
            }

            // 법선 변환용 3×3(translation 무시) — 호출부에서 inverse-transpose 행렬을 넘긴다.
            void TransformNormal(const aiMatrix4x4& _m, const aiVector3D& _n, float _out[3])
            {
                float x = _m.a1 * _n.x + _m.a2 * _n.y + _m.a3 * _n.z;
                float y = _m.b1 * _n.x + _m.b2 * _n.y + _m.b3 * _n.z;
                float z = _m.c1 * _n.x + _m.c2 * _n.y + _m.c3 * _n.z;
                const float len = std::sqrt(x * x + y * y + z * z);
                if (len > 1e-8f) { x /= len; y /= len; z /= len; }
                _out[0] = x; _out[1] = y; _out[2] = z;
            }

            // 노드 트리를 재귀 순회하며 각 메시를 월드공간으로 변환해 병합.
            void ProcessNode(const aiNode* _node, const aiMatrix4x4& _parentWorld,
                const aiScene* _scene,
                std::vector<MiniStaticVertex>& _verts,
                std::vector<uint32_t>& _indices)
            {
                const aiMatrix4x4 world = _parentWorld * _node->mTransformation;

                // 법선 행렬 = world 의 inverse-transpose(affine 이면 상단 3×3 = 3×3 inverse-transpose).
                aiMatrix4x4 normalMat = world;
                normalMat.Inverse().Transpose();

                for (unsigned int m = 0; m < _node->mNumMeshes; ++m)
                {
                    const aiMesh* mesh = _scene->mMeshes[_node->mMeshes[m]];
                    const uint32_t base = static_cast<uint32_t>(_verts.size());

                    for (unsigned int v = 0; v < mesh->mNumVertices; ++v)
                    {
                        MiniStaticVertex out = {};
                        TransformPoint(world, mesh->mVertices[v], out.position);

                        if (mesh->HasNormals())
                            TransformNormal(normalMat, mesh->mNormals[v], out.normal);
                        else
                        {
                            out.normal[0] = 0.0f; out.normal[1] = 1.0f; out.normal[2] = 0.0f;
                        }

                        if (mesh->HasTextureCoords(0))
                        {
                            out.uv[0] = mesh->mTextureCoords[0][v].x;
                            out.uv[1] = mesh->mTextureCoords[0][v].y;
                        }
                        _verts.push_back(out);
                    }

                    for (unsigned int f = 0; f < mesh->mNumFaces; ++f)
                    {
                        const aiFace& face = mesh->mFaces[f];
                        if (face.mNumIndices != 3) // Triangulate 후에도 방어적으로 삼각형만 수집
                            continue;
                        _indices.push_back(base + face.mIndices[0]);
                        _indices.push_back(base + face.mIndices[1]);
                        _indices.push_back(base + face.mIndices[2]);
                    }
                }

                for (unsigned int c = 0; c < _node->mNumChildren; ++c)
                    ProcessNode(_node->mChildren[c], world, _scene, _verts, _indices);
            }

            // ─── 스키닝 추출 헬퍼 ───────────────────────────────────────────

            // aiMatrix4x4(column-vector, v'=M·v) → SimpleMath Matrix(row-vector) — 전치 변환.
            // (translation: assimp 4열(a4,b4,c4) → SimpleMath 4행(_41,_42,_43).)
            Matrix ToMatrix(const aiMatrix4x4& _m)
            {
                return Matrix(
                    _m.a1, _m.b1, _m.c1, _m.d1,
                    _m.a2, _m.b2, _m.c2, _m.d2,
                    _m.a3, _m.b3, _m.c3, _m.d3,
                    _m.a4, _m.b4, _m.c4, _m.d4);
            }

            // 서브트리에 본 노드가 있으면 needed 로 마킹(조상 닫힘 — 본의 조상은 전부 포함).
            bool MarkNeededNodes(const aiNode* _node,
                const std::unordered_map<std::string, aiMatrix4x4>& _boneOffsets,
                std::unordered_set<const aiNode*>& _outNeeded)
            {
                bool needed = _boneOffsets.count(_node->mName.C_Str()) > 0;
                for (unsigned int c = 0; c < _node->mNumChildren; ++c)
                    needed |= MarkNeededNodes(_node->mChildren[c], _boneOffsets, _outNeeded);
                if (needed)
                    _outNeeded.insert(_node);
                return needed;
            }

            // needed 노드만 pre-order DFS 로 스켈레톤 배열에 추가 — 부모 인덱스 < 자기 인덱스 보장
            // (로더/Skeleton::ComputeBoneMatrices 전제). inverseBindPose 는 aiBone offset 우선,
            // offset 없는 조상 노드는 글로벌 바인드 누적의 역행렬로 계산.
            void BuildSkeleton(const aiNode* _node, int _parentIndex,
                const std::unordered_set<const aiNode*>& _needed,
                const std::unordered_map<std::string, aiMatrix4x4>& _boneOffsets,
                Skeleton& _outSkeleton,
                std::unordered_map<std::string, int>& _outIndexByName,
                std::vector<Matrix>& _globalBind)
            {
                if (_needed.count(_node) == 0)
                    return; // 서브트리에 본 없음 — 통째로 스킵

                const int myIndex = static_cast<int>(_outSkeleton.bones.size());

                Bone bone;
                bone.parentIndex = _parentIndex;
                bone.name = _node->mName.C_Str();
                bone.localBindPose = ToMatrix(_node->mTransformation);

                const Matrix global = (_parentIndex < 0)
                    ? bone.localBindPose
                    : bone.localBindPose * _globalBind[_parentIndex]; // row-vector: 자식 먼저
                _globalBind.push_back(global);

                const auto offsetIt = _boneOffsets.find(bone.name);
                bone.inverseBindPose = (offsetIt != _boneOffsets.end())
                    ? ToMatrix(offsetIt->second)
                    : global.Invert();

                _outSkeleton.bones.push_back(std::move(bone));
                _outIndexByName[_node->mName.C_Str()] = myIndex;

                for (unsigned int c = 0; c < _node->mNumChildren; ++c)
                    BuildSkeleton(_node->mChildren[c], myIndex, _needed, _boneOffsets,
                        _outSkeleton, _outIndexByName, _globalBind);
            }

            // 정점 1개에 (본, 가중치) 추가. 빈 슬롯 우선, 넘치면 최소 가중치 교체
            // (aiProcess_LimitBoneWeights 로 본당 4개 제한이라 통상 도달 안 함 — 방어용).
            void AddVertexWeight(MiniSkinnedVertex& _v, uint32_t _boneIndex, float _weight)
            {
                int smallest = 0;
                for (int s = 0; s < 4; ++s)
                {
                    if (_v.boneWeights[s] == 0.0f)
                    {
                        _v.boneIndices[s] = _boneIndex;
                        _v.boneWeights[s] = _weight;
                        return;
                    }
                    if (_v.boneWeights[s] < _v.boneWeights[smallest])
                        smallest = s;
                }
                if (_weight > _v.boneWeights[smallest])
                {
                    _v.boneIndices[smallest] = _boneIndex;
                    _v.boneWeights[smallest] = _weight;
                }
            }

            // 클립 이름 규칙: **파일 stem 이 1순위.** 소스 FBX 의 내부 트랙 이름은 건드리지 않고,
            // `.mini` 에 삽입할 때의 이름만 파일명으로 짓는다(언리얼/유니티 임포터 관례).
            //   익스포터의 기본 트랙 이름은 정보성이 없다 — Mixamo 는 전부 "mixamo.com",
            //   UE FBX 익스포트는 전부 "Unreal Take"(Assets\UE_Anim\ 44개 전부 동일). 이런 이름을
            //   junk 목록으로 하나씩 쫓아다니면 익스포터가 늘 때마다 깨지고, 무엇보다 **클립명이
            //   베이크 순서에 의존**하게 된다(첫 파일이 "Unreal Take" 를 선점 → 나머지는 중복
            //   fallback 으로 우연히 stem 을 받음, docs/UEFN_Bone.md §5.7). stem 을 정본으로 삼아
            //   이 문제를 근본에서 없앤다.
            //   _name(내부 이름)은 stem 을 구할 수 없을 때만 쓰는 예비 경로다.
            //   한 FBX 안에 트랙이 여러 개면 stem, stem_2, stem_3 … 으로 붙는다.
            //   (이 규칙은 신규 베이크에만 적용 — 이미 구운 .mini 는 저장된 이름을 그대로 유지한다.)
            std::string ResolveClipName(std::string _name, const std::string& _stem,
                const std::vector<AnimClip>& _clips)
            {
                const auto taken = [&_clips](const std::string& _n) {
                    for (const AnimClip& c : _clips)
                        if (c.name == _n)
                            return true;
                    return false;
                    };

                if (!_stem.empty())
                    _name = _stem;
                else if (_name.empty())
                    _name = "clip";
                if (!taken(_name))
                    return _name;
                for (int i = 2;; ++i)
                {
                    const std::string candidate = _name + "_" + std::to_string(i);
                    if (!taken(candidate))
                        return candidate;
                }
            }

            // 씬의 모든 애니메이션을 AnimClip 으로 추출해 _inoutClips 에 추가.
            // 채널은 _indexByName(본 이름→스켈레톤 인덱스)에 매칭되는 노드만 수집하고,
            // 매칭 실패 채널 수를 반환한다(카메라/타 스켈레톤 노드 등 — 호출부 판단).
            unsigned int ExtractClips(const aiScene* _scene,
                const std::unordered_map<std::string, int>& _indexByName,
                const std::string& _fallbackStem,
                std::vector<AnimClip>& _inoutClips)
            {
                unsigned int unmatched = 0;
                for (unsigned int a = 0; a < _scene->mNumAnimations; ++a)
                {
                    const aiAnimation* anim = _scene->mAnimations[a];

                    AnimClip clip;
                    clip.name = ResolveClipName(anim->mName.C_Str(), _fallbackStem, _inoutClips);
                    clip.duration = static_cast<float>(anim->mDuration);
                    clip.ticksPerSecond = (anim->mTicksPerSecond != 0.0)
                        ? static_cast<float>(anim->mTicksPerSecond) : 25.0f;

                    for (unsigned int c = 0; c < anim->mNumChannels; ++c)
                    {
                        const aiNodeAnim* nodeAnim = anim->mChannels[c];
                        const auto it = _indexByName.find(nodeAnim->mNodeName.C_Str());
                        if (it == _indexByName.end())
                        {
                            ++unmatched; // 스켈레톤 밖 노드(카메라 등) — 스킵
                            continue;
                        }

                        AnimChannel channel;
                        channel.boneIndex = it->second;
                        channel.pos.reserve(nodeAnim->mNumPositionKeys);
                        channel.rot.reserve(nodeAnim->mNumRotationKeys);
                        channel.scale.reserve(nodeAnim->mNumScalingKeys);
                        for (unsigned int k = 0; k < nodeAnim->mNumPositionKeys; ++k)
                        {
                            const aiVectorKey& key = nodeAnim->mPositionKeys[k];
                            channel.pos.push_back({ static_cast<float>(key.mTime),
                                Vector3(key.mValue.x, key.mValue.y, key.mValue.z) });
                        }
                        for (unsigned int k = 0; k < nodeAnim->mNumRotationKeys; ++k)
                        {
                            const aiQuatKey& key = nodeAnim->mRotationKeys[k];
                            // assimp aiQuaternion 은 멤버 접근(x/y/z/w)으로 성분 순서 안전.
                            channel.rot.push_back({ static_cast<float>(key.mTime),
                                Quaternion(key.mValue.x, key.mValue.y, key.mValue.z, key.mValue.w) });
                        }
                        for (unsigned int k = 0; k < nodeAnim->mNumScalingKeys; ++k)
                        {
                            const aiVectorKey& key = nodeAnim->mScalingKeys[k];
                            channel.scale.push_back({ static_cast<float>(key.mTime),
                                Vector3(key.mValue.x, key.mValue.y, key.mValue.z) });
                        }
                        clip.channels.push_back(std::move(channel));
                    }
                    _inoutClips.push_back(std::move(clip));
                }
                return unmatched;
            }

            // ─── 리타게팅 헬퍼 (베이크 타임, §9) ─────────────────────────────
            // 본 이름 정규화(NormalizeBoneName)와 휴머노이드 역할 사전(ResolveHumanoidBone)은
            // 런타임(루트 모션 본 탐색)과 공유하므로 Asset/BoneNaming 에 있다.

            // 소스 애니 FBX 의 노드 트리를 순회해 rest local/global 바인드 행렬을 이름별로 수집.
            // (스킨/애니 전용 FBX 모두 노드 mTransformation 은 존재 → aiBone 불필요, 견고.)
            void CollectRestPose(const aiNode* _node, const Matrix& _parentGlobal,
                std::unordered_map<std::string, Matrix>& _outLocal,
                std::unordered_map<std::string, Matrix>& _outGlobal)
            {
                const Matrix local = ToMatrix(_node->mTransformation);
                const Matrix global = local * _parentGlobal; // row-vector: 자식 먼저
                _outLocal[_node->mName.C_Str()] = local;
                _outGlobal[_node->mName.C_Str()] = global;
                for (unsigned int c = 0; c < _node->mNumChildren; ++c)
                    CollectRestPose(_node->mChildren[c], global, _outLocal, _outGlobal);
            }

            // 채널(소스 본) → 타깃 본 **자동** 매칭: (1) 정규화 이름 exact → (2) 휴머노이드 역할 fallback.
            // 반환 = 타깃 인덱스(-1=미매칭), _outKind 에 매칭 방식. (오버라이드는 호출부가 이 결과를 덮어씀.)
            // AnalyzeRetarget(진단) 과 RetargetAndExtractClips(추출) 가 공유하는 단일 매칭 출처.
            int MatchChannelToTarget(const std::string& _nodeName,
                const std::unordered_map<std::string, int>& _normalizedTargetIndex,
                const std::unordered_map<HumanoidBone, int>& _roleTargetIndex,
                RetargetChannelMatch::Kind& _outKind)
            {
                const std::string norm = NormalizeBoneName(_nodeName);
                if (const auto it = _normalizedTargetIndex.find(norm); it != _normalizedTargetIndex.end())
                {
                    _outKind = RetargetChannelMatch::Kind::Exact; // 동일 리그 무손실 경로
                    return it->second;
                }
                const HumanoidBone role = ResolveHumanoidBone(norm);
                if (role != HumanoidBone::None)
                    if (const auto rit = _roleTargetIndex.find(role); rit != _roleTargetIndex.end())
                    {
                        _outKind = RetargetChannelMatch::Kind::Role; // 크로스 리그 인식
                        return rit->second;
                    }
                _outKind = RetargetChannelMatch::Kind::Unmatched;
                return -1;
            }

            // 행렬에서 회전 성분만(스케일/이동 제거). 글로벌 회전 체인 계산용.
            Matrix RotationOnly(const Matrix& _m)
            {
                Vector3 s, t; Quaternion q;
                Matrix m = _m;            // Decompose 는 non-const → 로컬 복사본에서 분해
                m.Decompose(s, q, t);
                return Matrix::CreateFromQuaternion(q);
            }

            // ─── 리그 정렬(§9) ────────────────────────────────────────────────
            // 모델스페이스 리타게팅은 소스·타깃이 **같은 기준계**를 공유할 때만 성립한다. 실제 리그는
            // 두 가지가 어긋난다:
            //   (1) 월드 기저 — Mixamo 는 Y-up, UE 마네킹 FBX 는 Z-up 으로 임포트된다(측정: 88.9° 차).
            //   (2) rest 자세  — Mixamo 바인드는 T-pose, UE 마네킹은 A-pose(측정: 어깨 54.8°). UEFN
            //       트래버설 FBX 는 아예 서있지 않은 포즈가 rest 에 구워져 있다(측정: 허벅지 103°).
            // 둘 다 rest 포즈에서 유도해 흡수한다 — FBX 메타데이터(UpAxis)에 의존하지 않고 리그 무관.

            // 리그의 "캐릭터 공간" 기저 — rest 포즈에서 유도.
            //   up = hips→head, right = leftUpperLeg→rightUpperLeg, fwd = right×up (LH).
            //   B = char→world (row-vector: 행0=right, 행1=up, 행2=fwd). 직교정규 → inv = 전치.
            struct CharBasis
            {
                bool    valid = false;
                Vector3 right, up, fwd;
                Matrix  B;
            };

            CharBasis MakeCharBasis(const Vector3& _hips, const Vector3& _head,
                const Vector3& _leftLeg, const Vector3& _rightLeg)
            {
                CharBasis b;
                Vector3 up = _head - _hips;
                Vector3 r0 = _rightLeg - _leftLeg;
                if (up.Length() < 1e-5f || r0.Length() < 1e-5f) return b;
                up.Normalize(); r0.Normalize();
                Vector3 fwd = r0.Cross(up); // LH: right×up = forward
                if (fwd.Length() < 1e-5f) return b;   // up ∥ right (퇴화)
                fwd.Normalize();
                Vector3 right = up.Cross(fwd); // 재직교화(right 가 up 과 정확히 수직이 되도록)
                right.Normalize();
                b.valid = true; b.right = right; b.up = up; b.fwd = fwd;
                b.B = Matrix(right.x, right.y, right.z, 0.0f,
                    up.x, up.y, up.z, 0.0f,
                    fwd.x, fwd.y, fwd.z, 0.0f,
                    0.0f, 0.0f, 0.0f, 1.0f);
                return b;
            }

            // _from → _to 로 보내는 **최소(swing) 회전**. row-vector 규약: _from · M ≈ _to.
            // twist(축 둘레 롤)는 미결정 — 방향만 맞춘다(rest 정렬의 알려진 한계, §9).
            Matrix MinimalRotation(Vector3 _from, Vector3 _to)
            {
                if (_from.Length() < 1e-6f || _to.Length() < 1e-6f) return Matrix();
                _from.Normalize(); _to.Normalize();
                float d = _from.Dot(_to);
                d = (std::max)(-1.0f, (std::min)(1.0f, d));
                if (d > 0.99999f) return Matrix(); // 이미 일치 → 항등
                if (d < -0.99999f)
                {
                    // 정확히 반대 — 축이 미정이라 _from 에 수직인 아무 축이나 잡아 180° 회전.
                    Vector3 axis = Vector3(1.0f, 0.0f, 0.0f).Cross(_from);
                    if (axis.LengthSquared() < 1e-6f) axis = Vector3(0.0f, 1.0f, 0.0f).Cross(_from);
                    axis.Normalize();
                    return Matrix::CreateFromAxisAngle(axis, 3.14159265f);
                }
                Vector3 axis = _from.Cross(_to);
                axis.Normalize();
                return Matrix::CreateFromAxisAngle(axis, std::acos(d));
            }

            // 역할별 자식 후보(체인 위→아래). rest 사지 "방향"을 재려면 자식이 필요한데 리그마다
            // 분절 수가 달라(스파인 3 vs 5) 고정 매핑이 불가 → 소스·타깃 **둘 다** 가진 첫 후보를 쓴다.
            // 빈 목록(Head/Hand/Toes 등 말단)은 정렬을 부모에서 상속한다.
            std::vector<HumanoidBone> ChildRoleCandidates(HumanoidBone _r)
            {
                using H = HumanoidBone;
                switch (_r)
                {
                case H::Hips:          return { H::Spine, H::Chest, H::UpperChest, H::Neck, H::Head };
                case H::Spine:         return { H::Chest, H::UpperChest, H::Neck, H::Head };
                case H::Chest:         return { H::UpperChest, H::Neck, H::Head };
                case H::UpperChest:    return { H::Neck, H::Head };
                case H::Neck:          return { H::Head };
                case H::LeftShoulder:  return { H::LeftUpperArm };
                case H::RightShoulder: return { H::RightUpperArm };
                case H::LeftUpperArm:  return { H::LeftLowerArm };
                case H::RightUpperArm: return { H::RightLowerArm };
                case H::LeftLowerArm:  return { H::LeftHand };
                case H::RightLowerArm: return { H::RightHand };
                case H::LeftUpperLeg:  return { H::LeftLowerLeg };
                case H::RightUpperLeg: return { H::RightLowerLeg };
                case H::LeftLowerLeg:  return { H::LeftFoot };
                case H::RightLowerLeg: return { H::RightFoot };
                case H::LeftFoot:      return { H::LeftToes };
                case H::RightFoot:     return { H::RightToes };
                default:               return {};
                }
            }

            // 노드 트리를 pre-order 로 평탄화(이름·부모 인덱스). pre-order → 부모 인덱스 < 자기 인덱스
            // → 소스 글로벌 회전을 단일 전방 패스로 누적 가능(런타임 Skeleton 과 동일 전제).
            void FlattenNodes(const aiNode* _node, int _parent,
                std::vector<std::string>& _outNames, std::vector<int>& _outParents,
                std::unordered_map<std::string, int>& _outIndex)
            {
                const int me = static_cast<int>(_outNames.size());
                _outNames.push_back(_node->mName.C_Str());
                _outParents.push_back(_parent);
                _outIndex[_node->mName.C_Str()] = me;
                for (unsigned int c = 0; c < _node->mNumChildren; ++c)
                    FlattenNodes(_node->mChildren[c], me, _outNames, _outParents, _outIndex);
            }

            // aiNodeAnim 의 회전 키를 tick 에서 슬러프 보간(범위 밖은 끝 키로 클램프).
            Quaternion SampleAiRot(const aiNodeAnim* _na, double _t)
            {
                const unsigned int n = _na->mNumRotationKeys;
                if (n == 0) return Quaternion(); // 항등
                const auto toQ = [](const aiQuaternion& v) { return Quaternion(v.x, v.y, v.z, v.w); };
                if (_t <= _na->mRotationKeys[0].mTime)     return toQ(_na->mRotationKeys[0].mValue);
                if (_t >= _na->mRotationKeys[n - 1].mTime) return toQ(_na->mRotationKeys[n - 1].mValue);
                for (unsigned int k = 1; k < n; ++k)
                    if (_t < _na->mRotationKeys[k].mTime)
                    {
                        const double t0 = _na->mRotationKeys[k - 1].mTime;
                        const double t1 = _na->mRotationKeys[k].mTime;
                        const float  f = (t1 > t0) ? static_cast<float>((_t - t0) / (t1 - t0)) : 0.0f;
                        return Quaternion::Slerp(toQ(_na->mRotationKeys[k - 1].mValue),
                            toQ(_na->mRotationKeys[k].mValue), f);
                    }
                return toQ(_na->mRotationKeys[n - 1].mValue);
            }

            // aiNodeAnim 의 위치 키를 tick 에서 선형 보간(범위 밖은 끝 키로 클램프). 루트 모션
            // 소스 hip 글로벌 궤적을 프레임 정합으로 뽑기 위한 이동 샘플러(회전은 SampleAiRot).
            aiVector3D SampleAiPos(const aiNodeAnim* _na, double _t)
            {
                const unsigned int n = _na->mNumPositionKeys;
                if (n == 0) return aiVector3D(0.0f, 0.0f, 0.0f);
                if (_t <= _na->mPositionKeys[0].mTime)     return _na->mPositionKeys[0].mValue;
                if (_t >= _na->mPositionKeys[n - 1].mTime) return _na->mPositionKeys[n - 1].mValue;
                for (unsigned int k = 1; k < n; ++k)
                    if (_t < _na->mPositionKeys[k].mTime)
                    {
                        const double t0 = _na->mPositionKeys[k - 1].mTime;
                        const double t1 = _na->mPositionKeys[k].mTime;
                        const float  f = (t1 > t0) ? static_cast<float>((_t - t0) / (t1 - t0)) : 0.0f;
                        const aiVector3D& a = _na->mPositionKeys[k - 1].mValue;
                        const aiVector3D& b = _na->mPositionKeys[k].mValue;
                        return a + (b - a) * f;
                    }
                return _na->mPositionKeys[n - 1].mValue;
            }

            // ─── 리타게팅 품질 진단(§5.6 지표) ────────────────────────────────
            // RetargetAndExtractClips 가 **실제 베이크 경로 위에서** 채워주는 선택적 출력.
            // 별도 함수로 수학을 복제하지 않는 이유: 복제본은 본 경로와 조용히 어긋날 수 있고,
            // 그러면 지표가 "복제본이 자기 자신과 일치한다"만 재는 무의미한 값이 된다.
            //
            // limbAngle = 소스 사지 방향 vs 리타게팅 결과 사지 방향(**각자의 캐릭터 공간**) 각도.
            // 리타게팅 공식이 목표로 하는 항등 `childDir_t·Gt ≡ childDir_s·Gs`(위 주석 참조)의
            // 잔차이므로 0 이 정답이다. 실측 이력(UEFN_Bone.md §5.6): 깨졌을 때 149°/170°,
            // 고친 뒤 0.02°. aligned=false(역할 본 부재로 기저 유도 실패)면 측정 불가 → -1.
            struct RetargetDiag
            {
                bool    aligned = false;
                float   heightRatio = 1.0f;
                float   maxLimbAngleDeg = -1.0f;
                float   meanLimbAngleDeg = -1.0f;
                int     limbSamples = 0; // (사지 구간 × 프레임) 표본 수, 0 = 측정 안 됨
                Vector3 srcUp;                // 소스 rest 의 캐릭터 up(= Bs.up) — 축 진단용
                Vector3 rootMotionNet;        // 루트모션 순변위(방출 공간 = 타깃 루트본 부모 로컬)
            };

            // 지표를 재는 사지 구간 — 각 역할의 rest 자식 방향이 정의되는 본들(ChildRoleCandidates).
            // 스파인/목은 다대일 근사(spine_02~05→UpperChest)라 잔차가 구조적으로 남으므로 제외하고,
            // 리그 차가 가장 크게 드러나는 사지만 본다(§5.6 이 측정한 것과 동일 집합).
            const HumanoidBone kLimbRoles[] = {
                HumanoidBone::LeftUpperArm,  HumanoidBone::LeftLowerArm,
                HumanoidBone::RightUpperArm, HumanoidBone::RightLowerArm,
                HumanoidBone::LeftUpperLeg,  HumanoidBone::LeftLowerLeg,
                HumanoidBone::RightUpperLeg, HumanoidBone::RightLowerLeg,
            };

            // 추가 애니 소스를 타깃 스켈레톤으로 "리타게팅"해 클립으로 추출·병합.
            //  - 채널→본 매칭: _overrides(소스본이름→타깃인덱스, -1=스킵) 우선 → 없으면 자동
            //          (MatchChannelToTarget: 정규화 이름 exact → 휴머노이드 역할 fallback). 둘 다 실패 = 미매칭(수 반환).
            //  - 회전: **모델스페이스 방향 리타게팅 + 리그 정렬**(언리얼 IK Retargeter/유니티 휴머노이드 정석).
            //          목표는 "타깃 본이 소스 본과 **같은 방향을 가리키게** 한다" 이며, 본별 상수 하나로 접힌다:
            //              Gt^w = Kpre · Gs^w · Kpost
            //              Kpre  = GtRest^w · inv(Bt) · S · Bs · inv(GsRest^w)
            //              Kpost = inv(Bs) · Bt
            //              local = Gt^w · inv(부모 Gt^w)      (row-vector, global = local·parent)
            //          B  = 리그의 캐릭터 공간 기저(rest 에서 유도, MakeCharBasis) — Y-up↔Z-up 기저 차 흡수.
            //          S  = 타깃 rest 사지 방향 → 소스 rest 사지 방향 최소 회전 — T-pose↔A-pose 및 rest 에
            //               임의 포즈가 구워진 UEFN FBX 를 흡수(= 자동 Retarget Pose / 유니티 Enforce T-Pose).
            //          이 조합은 childDir_t·Gt ≡ childDir_s·Gs 로 떨어져 **소스의 절대 사지 방향을 재현**한다.
            //          **동일 리그면 Bs=Bt, GsRest=GtRest, S=I → Kpre=Kpost=I → Gt=Gs (무회귀 항등).**
            //          ⚠ S 는 swing 만 정의 — 축 둘레 twist(손/발 롤)는 미결정(문서화된 한계).
            //          [폐기 이력] ① 채널별 로컬 바인드 델타(qFix) → 크로스 리그 방향 실패(캐릭터가 누움).
            //            ② `Gs·inv(GsRest)·GtRest` → **column-vector 공식을 row-vector 엔진에 적용**한 것이라
            //               실제로는 "본 로컬 프레임 델타" 전이였다. 동일 리그는 항등이라 회귀 검사를 통과했으나,
            //               본 축 규약이 다른 크로스 리그(Mixamo Y-down-bone ↔ UE X-down-bone, 측정 90°)에서
            //               사지가 엉뚱한 평면으로 회전했다.
            //  - 트랜슬레이션(루트모션): 소스 hip(역할==Hips) 본의 **글로벌 위치**(부모 root 의 전진/수직
            //          이동 포함)를 매 tick 뽑아 타깃 루트모션 본(FindRootMotionBone)에 방출한다. UE 는 이동을
            //          전용 root 에 저작 → hip 로컬만 쓰면 전진이 소실되므로 글로벌로 합성한다. 나머지 본 pos 는
            //          방출 안 함(런타임 바인드 fallback). scale 드롭.
            //          **기저 정렬(Kpost)로 소스 월드 → 타깃 월드 변환**하고, 높이 비율은 각 리그의 **up 축
            //          투영**(rest hip 높이)으로 낸다 — 월드 Y 를 쓰면 Z-up 리그에서 무의미한 값이 나온다
            //          (마네킹 측정: tgtHipY=-0.023 → ratio -0.023 으로 깨져 있었다).
            //          런타임이 XZ(+yaw)만 추출·strip → 수직 hop 은 포즈에 남아 몸이 떠오른다.
            //          (타깃이 전용 root 본을 갖는 케이스는 hip 글로벌을 root 에 실으면 수직 오프셋 → 후속 과제.)
            //          **축 규약**: 방출 공간은 타깃 rest 를 유도한 공간 = 노드 체인(2번 주석 참조)이다.
            //          BakeSkinned 의 NormalizeSkeletonSpace 후처리가 루트모션 본의 조상을 접어
            //          "루트 본 local == 메시 공간" 을 성립시키므로 RootMotion.h 의 Y-up·메시로컬 전제가
            //          실제로 참이 된다(구: 마네킹이 −Z-up 으로 구워져 전진이 소실됐다 — UEFN_Bone.md §5.8).
            unsigned int RetargetAndExtractClips(
                const aiScene* _animScene,
                const Skeleton& _targetSkeleton,
                const std::unordered_map<std::string, int>& _normalizedTargetIndex,
                const std::unordered_map<HumanoidBone, int>& _roleTargetIndex,
                const std::string& _fallbackStem,
                std::vector<AnimClip>& _inoutClips,
                const std::unordered_map<std::string, int>* _overrides = nullptr,
                RetargetDiag* _outDiag = nullptr,
                // 소스 애셋 전체에 적용할 월드 회전(= 소스 루트의 부모). Up Axis 오버라이드가
                // UpAxisMatrix 로 넘겨준다. 기본 항등 → 소스 raw 그대로(무영향). 회전만이므로
                // 원점 기준 월드 후치 회전(row-vector: global = local·…·root·N). ⚠ **정렬된
                // 휴머노이드 소스는 무영향** — 기저 Bs 가 rest 에서 유도돼 이미 축을 흡수한다.
                // raw 모델스페이스 폴백(역할 본 부재)에서만 실효.
                const Matrix& _srcWorldRot = Matrix())
            {
                // 1) 소스 계층(순서·부모)·rest 포즈 수집. FlattenNodes 는 pre-order 라 부모 인덱스 < 자기
                //    인덱스 → rest/anim 글로벌 회전을 단일 전방 패스로 누적할 수 있다.
                std::vector<std::string> srcNames;
                std::vector<int>         srcParents;
                std::unordered_map<std::string, int> srcIndex;
                FlattenNodes(_animScene->mRootNode, -1, srcNames, srcParents, srcIndex);

                std::unordered_map<std::string, Matrix> srcLocal, srcGlobal;
                CollectRestPose(_animScene->mRootNode, _srcWorldRot, srcLocal, srcGlobal);

                const size_t srcCount = srcNames.size();
                std::vector<Matrix> srcRestLocalRot(srcCount);      // 회전만(스케일/이동 제거)
                std::vector<Matrix> srcRestGlobalRot(srcCount);     // GsRest — anim 과 동일 체인으로 계산
                std::vector<Vector3> srcRestGlobalPos(srcCount);    // rest 위치(사지 방향/기저 유도용)
                std::unordered_map<HumanoidBone, int> srcRoleIndex; // 역할 → 소스 인덱스(첫 등장 우선)
                for (size_t i = 0; i < srcCount; ++i)
                {
                    srcRestLocalRot[i] = RotationOnly(srcLocal[srcNames[i]]);
                    srcRestGlobalPos[i] = srcGlobal[srcNames[i]].Translation();
                    const int p = srcParents[i];
                    srcRestGlobalRot[i] = (p < 0)
                        ? srcRestLocalRot[i] * _srcWorldRot : srcRestLocalRot[i] * srcRestGlobalRot[p];
                    const HumanoidBone role = ResolveHumanoidBone(NormalizeBoneName(srcNames[i]));
                    if (role != HumanoidBone::None)
                        srcRoleIndex.emplace(role, static_cast<int>(i));
                }

                // 2) 타깃 rest(로컬·글로벌) 미리 계산. GtRest = **localBindPose 부모 체인 누적**.
                //    ⚠ inverseBindPose.Invert() 를 쓰면 안 된다 — 둘은 **다른 공간**이다:
                //      · localBindPose = aiNode::mTransformation. 노드 체인은 assimp 가 FBX UpAxis
                //        메타데이터로 만든 **축 변환 A** 를 RootNode 에 담고 있다(마네킹 실측 A=R_x(+90°)).
                //      · inverseBindPose = aiBone::mOffsetMatrix. assimp 는 여기에 **A 를 적용하지 않는다**.
                //        (실측: 모든 스킨 본에서 inverseBindPose·globalBind = A ≠ I)
                //    이 자체는 정상이다 — 스키닝 계약 final = invBind·G = raw⁻¹·raw·A = A 라서 A 가 노드
                //    체인을 통해 정점에 도달한다. 문제는 rest 를 invBind⁻¹(=raw, A 가 빠진 변환 전 공간)에서
                //    유도하면 리타게팅 클립이 **바인드 포즈와 다른 공간**(마네킹의 경우 −Z-up)에 저작된다는 것.
                //    노드 체인으로 유도하면 Kpre 는 불변이고 Kpost 만 ·A 를 얻어 회전 채널과 루트모션 pos
                //    트랙이 **함께** 바인드 포즈와 같은 공간으로 옮겨간다. Y-up 리그는 A=I 라 무회귀.
                //    (BuildSkeleton 이 pre-order → parentIndex < 자기 인덱스 보장 → 단일 전방 패스.)
                const size_t tgtCount = _targetSkeleton.bones.size();
                std::vector<Matrix>  tgtRestLocalRot(tgtCount), tgtRestGlobalRot(tgtCount);
                std::vector<Vector3> tgtRestGlobalPos(tgtCount);
                std::vector<Matrix>  tgtRestGlobal(tgtCount);
                for (size_t i = 0; i < tgtCount; ++i)
                {
                    const int p = _targetSkeleton.bones[i].parentIndex;
                    tgtRestGlobal[i] = (p < 0)
                        ? _targetSkeleton.bones[i].localBindPose
                        : _targetSkeleton.bones[i].localBindPose * tgtRestGlobal[p];
                    tgtRestLocalRot[i] = RotationOnly(_targetSkeleton.bones[i].localBindPose);
                    tgtRestGlobalRot[i] = RotationOnly(tgtRestGlobal[i]);
                    tgtRestGlobalPos[i] = tgtRestGlobal[i].Translation();
                }

                // 3) 리그 정렬 — 캐릭터 공간 기저(Bs/Bt). 실패(역할 본 부재)하면 항등으로 두어
                //    기존 동작(동일 리그 경로)을 유지한다.
                const auto restPos = [](const std::unordered_map<HumanoidBone, int>& _roles,
                    const std::vector<Vector3>& _pos, HumanoidBone _r, Vector3& _out) {
                        const auto it = _roles.find(_r);
                        if (it == _roles.end()) return false;
                        _out = _pos[it->second]; return true;
                    };
                CharBasis Bs, Bt;
                {
                    Vector3 h, hd, l, r;
                    if (restPos(srcRoleIndex, srcRestGlobalPos, HumanoidBone::Hips, h)
                        && restPos(srcRoleIndex, srcRestGlobalPos, HumanoidBone::Head, hd)
                        && restPos(srcRoleIndex, srcRestGlobalPos, HumanoidBone::LeftUpperLeg, l)
                        && restPos(srcRoleIndex, srcRestGlobalPos, HumanoidBone::RightUpperLeg, r))
                        Bs = MakeCharBasis(h, hd, l, r);
                    if (restPos(_roleTargetIndex, tgtRestGlobalPos, HumanoidBone::Hips, h)
                        && restPos(_roleTargetIndex, tgtRestGlobalPos, HumanoidBone::Head, hd)
                        && restPos(_roleTargetIndex, tgtRestGlobalPos, HumanoidBone::LeftUpperLeg, l)
                        && restPos(_roleTargetIndex, tgtRestGlobalPos, HumanoidBone::RightUpperLeg, r))
                        Bt = MakeCharBasis(h, hd, l, r);
                }
                const bool aligned = Bs.valid && Bt.valid;
                if (!aligned)
                    MG_LOG_WARN("AssimpBaker: rig alignment unavailable (missing Hips/Head/UpperLeg role) "
                        "— falling back to raw model-space transfer");
                const Matrix BsM = aligned ? Bs.B : Matrix();
                const Matrix BtM = aligned ? Bt.B : Matrix();
                const Matrix BsInv = BsM.Invert();
                const Matrix BtInv = BtM.Invert();
                const Matrix Kpost = BsInv * BtM;  // 소스 월드 → 타깃 월드 (루트모션 이동과 공용)

                // rest 사지 방향(캐릭터 공간) — 본별 정렬 S 의 입력. 소스·타깃 **둘 다** 자식 역할을
                // 가진 본만 산출된다(말단은 부모에서 상속).
                std::unordered_map<HumanoidBone, Vector3> srcRestDirChar, tgtRestDirChar;
                // 같은 방향의 **월드(모델) 공간** 원본 — 진단 지표(_outDiag)에서 애니 프레임의 사지
                // 방향을 재구성할 때 쓴다(rest 월드 방향 → 본 rest 로컬 → 애니 글로벌 적용).
                std::unordered_map<HumanoidBone, Vector3> srcRestDirWorld, tgtRestDirWorld;
                if (aligned)
                {
                    for (const auto& kv : srcRoleIndex)
                    {
                        const auto tit = _roleTargetIndex.find(kv.first);
                        if (tit == _roleTargetIndex.end()) continue;
                        for (const HumanoidBone childRole : ChildRoleCandidates(kv.first))
                        {
                            const auto sc = srcRoleIndex.find(childRole);
                            const auto tc = _roleTargetIndex.find(childRole);
                            if (sc == srcRoleIndex.end() || tc == _roleTargetIndex.end())
                                continue; // 이 후보는 한쪽에만 있음 → 다음 후보
                            Vector3 sd = srcRestGlobalPos[sc->second] - srcRestGlobalPos[kv.second];
                            Vector3 td = tgtRestGlobalPos[tc->second] - tgtRestGlobalPos[tit->second];
                            if (sd.Length() < 1e-5f || td.Length() < 1e-5f) continue;
                            Vector3 sw = sd; sw.Normalize();
                            Vector3 tw = td; tw.Normalize();
                            srcRestDirWorld[kv.first] = sw;
                            tgtRestDirWorld[kv.first] = tw;
                            sd = Vector3::TransformNormal(sd, BsInv); sd.Normalize();
                            td = Vector3::TransformNormal(td, BtInv); td.Normalize();
                            srcRestDirChar[kv.first] = sd;
                            tgtRestDirChar[kv.first] = td;
                            break; // 첫 유효 후보 채택
                        }
                    }
                }

                // 소스 hip(역할==Hips) 본 인덱스 + 타깃 루트모션 본(런타임과 동일 규칙). 소스 hip 의
                // 글로벌 궤적(부모 root 의 전진/수직 이동 포함)을 타깃 루트모션 본 이동으로 전이한다.
                const auto hipIt = srcRoleIndex.find(HumanoidBone::Hips);
                const int  srcHipsIdx = (hipIt != srcRoleIndex.end()) ? hipIt->second : -1;
                const int  tgtRootMotionBone = FindRootMotionBone(_targetSkeleton);
                const bool emitRootMotion = (srcHipsIdx >= 0 && tgtRootMotionBone >= 0);

                // 루트모션 본의 **부모 글로벌 역행렬** — 방출할 월드 위치를 로컬 키로 환산한다.
                // 조상(RootNode/Armature 등)은 역할이 없어 매칭되지 않으므로 애니되지 않는다 →
                // 부모 글로벌 = 바인드 글로벌(시간 불변 상수). BakeSkinned 의 NormalizeSkeletonSpace 가
                // 이 조상들을 항등으로 접으면 결국 local == 메시 공간이 된다.
                Matrix rootMotionParentInv; // 항등
                if (emitRootMotion)
                {
                    const int p = _targetSkeleton.bones[tgtRootMotionBone].parentIndex;
                    if (p >= 0)
                        rootMotionParentInv = tgtRestGlobal[p].Invert();
                }

                // 루트 모션 이동 스케일 = 소스/타깃 rest hip **높이 비**. 높이는 각 리그의 up 축 투영으로
                // 잰다 — 월드 Y 는 Z-up 리그(UE 마네킹)에서 높이가 아니다. 동일 리그면 ≈1(항등).
                float heightRatio = 1.0f;
                if (aligned && srcHipsIdx >= 0)
                {
                    const auto tHip = _roleTargetIndex.find(HumanoidBone::Hips);
                    if (tHip != _roleTargetIndex.end())
                    {
                        const float sH = srcRestGlobalPos[srcHipsIdx].Dot(Bs.up);
                        const float tH = tgtRestGlobalPos[tHip->second].Dot(Bt.up);
                        if (std::fabs(sH) > 1e-4f && std::fabs(tH) > 1e-4f)
                            heightRatio = tH / sH;
                    }
                }

                unsigned int unmatched = 0;
                unsigned int retargetedChannels = 0;

                // 진단 지표 누적기(_outDiag 요청 시에만 채워짐).
                double diagAngleSum = 0.0;
                float  diagAngleMax = 0.0f;
                int    diagSamples = 0;

                for (unsigned int a = 0; a < _animScene->mNumAnimations; ++a)
                {
                    const aiAnimation* anim = _animScene->mAnimations[a];

                    AnimClip clip;
                    clip.name = ResolveClipName(anim->mName.C_Str(), _fallbackStem, _inoutClips);
                    clip.duration = static_cast<float>(anim->mDuration);
                    clip.ticksPerSecond = (anim->mTicksPerSecond != 0.0)
                        ? static_cast<float>(anim->mTicksPerSecond) : 25.0f;

                    // 노드이름→채널 맵 + 샘플 타임 합집합(전 채널 회전 키 시간, 정렬·중복제거).
                    std::unordered_map<std::string, const aiNodeAnim*> nodeAnimByName;
                    std::set<double> timeSet;
                    for (unsigned int c = 0; c < anim->mNumChannels; ++c)
                    {
                        const aiNodeAnim* na = anim->mChannels[c];
                        nodeAnimByName[na->mNodeName.C_Str()] = na;
                        for (unsigned int k = 0; k < na->mNumRotationKeys; ++k)
                            timeSet.insert(na->mRotationKeys[k].mTime);
                    }

                    // 타깃 본 → 소스 노드 역맵. 매칭은 소스 채널 순회로 계산하되 타깃 기준으로 뒤집는다
                    // (타깃 로컬 환산이 부모→자식 topo 순을 요구). 다대일(UE spine_02~05→UpperChest,
                    // neck_01/02→Neck) 은 **가장 깊은 소스**(FlattenNodes pre-order 인덱스 최대 = 체인 최상단)
                    // 를 채택 — 모델스페이스 회전이 소스 글로벌을 타깃 글로벌에 재적용하므로 상단 척추의
                    // 누적 굽힘을 타깃 상단 본에 실어야 상체 방향이 충실히 재현된다. exact 이름 매칭(동일 리그)
                    // 은 타깃당 소스 유일 → 항등 무영향. override 는 지정 인덱스 그대로.
                    std::vector<int> tgtToSrc(tgtCount, -1);
                    for (unsigned int c = 0; c < anim->mNumChannels; ++c)
                    {
                        const std::string nodeName = anim->mChannels[c]->mNodeName.C_Str();
                        RetargetChannelMatch::Kind kind;
                        int targetBoneIndex = MatchChannelToTarget(
                            nodeName, _normalizedTargetIndex, _roleTargetIndex, kind);
                        if (_overrides)
                            if (const auto ov = _overrides->find(nodeName); ov != _overrides->end())
                                targetBoneIndex = ov->second; // 사용자 지정(-1 = 스킵)
                        if (targetBoneIndex < 0 || targetBoneIndex >= static_cast<int>(tgtCount))
                        {
                            ++unmatched; // 미매칭(여분 노드) 또는 사용자가 스킵 지정
                            continue;
                        }
                        const auto si = srcIndex.find(nodeName);
                        if (si == srcIndex.end()) { ++unmatched; continue; }
                        // 더 깊은(체인 상단) 소스가 이미 잡혔으면 유지(첫-우선 아님, 최심-우선).
                        if (tgtToSrc[targetBoneIndex] >= 0 && si->second <= tgtToSrc[targetBoneIndex])
                            continue;
                        tgtToSrc[targetBoneIndex] = si->second;
                    }

                    // 본별 정렬 상수 Kpre = GtRest^w · inv(Bt) · S · Bs · inv(GsRest^w).
                    //   S = 타깃 rest 사지 방향 → 소스 rest 사지 방향 최소 회전(캐릭터 공간).
                    //   자식 역할이 없어 S 를 못 내는 말단 본(Head/Hand/Toes/root/twist)은 **부모에서 상속**
                    //   한다 — 타깃 본은 topo 순(부모 < 자기)이라 전방 1패스로 해결된다. 상속까지 없으면 I.
                    std::vector<Matrix> boneSwing(tgtCount);       // S (캐릭터 공간)
                    std::vector<bool>   hasOwnSwing(tgtCount, false);
                    if (aligned)
                    {
                        for (size_t tb = 0; tb < tgtCount; ++tb)
                        {
                            const HumanoidBone role =
                                ResolveHumanoidBone(NormalizeBoneName(_targetSkeleton.bones[tb].name));
                            if (role == HumanoidBone::None) continue;
                            // 이 타깃 본이 그 역할의 대표(첫 등장)일 때만 — 다대일의 중복 본은 상속받는다.
                            const auto rep = _roleTargetIndex.find(role);
                            if (rep == _roleTargetIndex.end() || rep->second != static_cast<int>(tb)) continue;
                            const auto sd = srcRestDirChar.find(role);
                            const auto td = tgtRestDirChar.find(role);
                            if (sd == srcRestDirChar.end() || td == tgtRestDirChar.end()) continue;
                            boneSwing[tb] = MinimalRotation(td->second, sd->second);
                            hasOwnSwing[tb] = true;
                        }
                        for (size_t tb = 0; tb < tgtCount; ++tb) // 상속 패스(부모 < 자기 보장)
                            if (!hasOwnSwing[tb])
                            {
                                const int p = _targetSkeleton.bones[tb].parentIndex;
                                boneSwing[tb] = (p >= 0) ? boneSwing[p] : Matrix();
                            }
                    }

                    std::vector<Matrix> Kpre(tgtCount);
                    for (size_t tb = 0; tb < tgtCount; ++tb)
                    {
                        const int sb = tgtToSrc[tb];
                        if (sb < 0) continue;
                        Kpre[tb] = tgtRestGlobalRot[tb] * BtInv * boneSwing[tb] * BsM
                            * srcRestGlobalRot[sb].Invert();
                    }

                    // 매칭된 타깃 본별 회전 채널 누적기.
                    std::vector<AnimChannel> tgtChannels(tgtCount);
                    std::vector<bool>        used(tgtCount, false);
                    for (size_t i = 0; i < tgtCount; ++i) tgtChannels[i].boneIndex = static_cast<int>(i);

                    std::vector<Matrix> srcGlobalRot(srcCount);  // 프레임별 소스 애니 글로벌 회전
                    std::vector<Matrix> srcGlobalFull(srcCount); // 프레임별 소스 애니 글로벌(이동 포함, 루트모션용)
                    std::vector<Matrix> tgtGlobalRot(tgtCount);  // 프레임별 타깃 애니 글로벌 회전

                    for (double tick : timeSet)
                    {
                        // (a) 소스 애니 로컬→글로벌 회전(키 있으면 슬러프, 없으면 rest 로컬). 회전 리타게팅용.
                        //     동시에 이동 포함 풀 글로벌(srcGlobalFull)을 누적 — 루트모션 hip 궤적 추출용.
                        for (size_t i = 0; i < srcCount; ++i)
                        {
                            const int p = srcParents[i];
                            Matrix localRot;
                            const auto na = nodeAnimByName.find(srcNames[i]);
                            const aiNodeAnim* naPtr = (na != nodeAnimByName.end()) ? na->second : nullptr;
                            if (naPtr && naPtr->mNumRotationKeys > 0)
                                localRot = Matrix::CreateFromQuaternion(SampleAiRot(naPtr, tick));
                            else
                                localRot = srcRestLocalRot[i];
                            srcGlobalRot[i] = (p < 0) ? localRot * _srcWorldRot : localRot * srcGlobalRot[p];

                            if (emitRootMotion)
                            {
                                // 풀 로컬 = rest scale · anim rot · anim trans (row-vector S·R·T, 키 없으면 rest).
                                const Matrix& restL = srcLocal[srcNames[i]];
                                Vector3 rs, rt; Quaternion rq;
                                { Matrix m = restL; m.Decompose(rs, rq, rt); }
                                const bool hasRot = naPtr && naPtr->mNumRotationKeys > 0;
                                const bool hasPos = naPtr && naPtr->mNumPositionKeys > 0;
                                Matrix localFull;
                                if (hasRot || hasPos)
                                {
                                    const Quaternion q = hasRot ? SampleAiRot(naPtr, tick) : rq;
                                    const aiVector3D  t = hasPos ? SampleAiPos(naPtr, tick)
                                        : aiVector3D(rt.x, rt.y, rt.z);
                                    localFull = Matrix::CreateScale(rs)
                                        * Matrix::CreateFromQuaternion(q)
                                        * Matrix::CreateTranslation(t.x, t.y, t.z);
                                }
                                else
                                    localFull = restL;
                                srcGlobalFull[i] = (p < 0) ? localFull * _srcWorldRot : localFull * srcGlobalFull[p];
                            }
                        }
                        // 루트모션: 소스 hip 글로벌 위치(부모 root 이동 포함)를 타깃 루트모션 본 이동으로 전이.
                        // Kpost 로 소스 월드 → 타깃 월드 기저 변환 + 높이 비율 스케일(타깃 프로포션 정합).
                        // 런타임이 XZ(+yaw)만 추출·strip 한다.
                        if (emitRootMotion)
                        {
                            Vector3 hipG = srcGlobalFull[srcHipsIdx].Translation();
                            hipG = Vector3::TransformNormal(hipG, Kpost); // 기저 정렬(동일 리그면 항등)
                            hipG *= heightRatio;
                            // 월드 → **부모 로컬**. 채널 키는 로컬이므로 아래 회전 방출이
                            // `desiredG · inv(parentG)` 로 환산하는 것과 동일하게 맞춰야 한다.
                            // (구 코드는 이 환산을 빠뜨렸고, 그 오류가 "타깃 rest 를 invBind⁻¹(=축 변환 A 가
                            //  빠진 raw 공간)에서 유도" 하던 또 다른 오류와 **정확히 상쇄**돼 포즈만 맞아 보였다.
                            //  두 오류가 상쇄된 탓에 방출 트랙 자체는 raw 공간에 남았고, 채널을 직접 읽는
                            //  ExtractClipRootMotion 의 Y-up 전제가 깨져 있었다 — UEFN_Bone.md §5.8.)
                            hipG = Vector3::Transform(hipG, rootMotionParentInv);
                            tgtChannels[tgtRootMotionBone].pos.push_back(
                                { static_cast<float>(tick), hipG });
                            used[tgtRootMotionBone] = true;
                        }
                        // (b) 타깃: 부모 먼저(topo) 애니 글로벌 계산 + 매칭 본 로컬 회전 방출.
                        //     Gt^w = Kpre · Gs^w · Kpost — 소스의 절대 사지 방향을 타깃에 재현.
                        for (size_t tb = 0; tb < tgtCount; ++tb)
                        {
                            const int p = _targetSkeleton.bones[tb].parentIndex;
                            const Matrix parentG = (p < 0) ? Matrix() : tgtGlobalRot[p];
                            const int sb = tgtToSrc[tb];
                            if (sb >= 0)
                            {
                                const Matrix desiredG = Kpre[tb] * srcGlobalRot[sb] * Kpost;
                                const Matrix localRot = desiredG * parentG.Invert(); // Gt·inv(부모)
                                tgtGlobalRot[tb] = desiredG;
                                Quaternion q = Quaternion::CreateFromRotationMatrix(localRot);
                                q.Normalize();
                                tgtChannels[tb].rot.push_back({ static_cast<float>(tick), q });
                                used[tb] = true;
                            }
                            else
                            {
                                // 미매칭 본: 타깃 rest 로컬 유지(자식 환산용 글로벌만 누적, 키 방출 안 함).
                                tgtGlobalRot[tb] = tgtRestLocalRot[tb] * parentG;
                            }
                        }

                        // (c) 진단: 이 프레임의 사지 방향 잔차. 소스·타깃 글로벌이 둘 다 살아있는
                        //     지금 재야 한다(밖에서 재현하려면 위 수학을 통째로 복제해야 한다).
                        //     각 사지의 rest 월드 방향을 본 rest 로컬로 되돌린 뒤 애니 글로벌을 적용해
                        //     현재 방향을 얻고, 각자의 캐릭터 공간으로 투영해 비교한다.
                        if (_outDiag && aligned)
                        {
                            for (const HumanoidBone role : kLimbRoles)
                            {
                                const auto tit = _roleTargetIndex.find(role);
                                const auto sit = srcRoleIndex.find(role);
                                if (tit == _roleTargetIndex.end() || sit == srcRoleIndex.end()) continue;
                                const int tb = tit->second;
                                const int sb = sit->second;
                                if (tb < 0 || tgtToSrc[tb] < 0) continue; // 이 본은 리타게팅 안 됨
                                const auto sw = srcRestDirWorld.find(role);
                                const auto tw = tgtRestDirWorld.find(role);
                                if (sw == srcRestDirWorld.end() || tw == tgtRestDirWorld.end()) continue;

                                // rest 월드 방향 → 본 rest 로컬 프레임 → 애니 글로벌 → 캐릭터 공간.
                                Vector3 sDir = Vector3::TransformNormal(
                                    Vector3::TransformNormal(sw->second, srcRestGlobalRot[sb].Invert()),
                                    srcGlobalRot[sb]);
                                Vector3 tDir = Vector3::TransformNormal(
                                    Vector3::TransformNormal(tw->second, tgtRestGlobalRot[tb].Invert()),
                                    tgtGlobalRot[tb]);
                                sDir = Vector3::TransformNormal(sDir, BsInv);
                                tDir = Vector3::TransformNormal(tDir, BtInv);
                                if (sDir.Length() < 1e-5f || tDir.Length() < 1e-5f) continue;
                                sDir.Normalize(); tDir.Normalize();

                                float d = sDir.Dot(tDir);
                                d = (std::max)(-1.0f, (std::min)(1.0f, d));
                                const float deg = std::acos(d) * 57.2957795f;
                                diagAngleSum += deg;
                                diagAngleMax = (std::max)(diagAngleMax, deg);
                                ++diagSamples;
                            }
                        }
                    }

                    // (루트모션 pos 는 위 tick 루프에서 타깃 루트모션 본에 이미 방출됨.)

                    // (d) 매칭(사용된) 채널만 클립에 추가.
                    for (size_t tb = 0; tb < tgtCount; ++tb)
                        if (used[tb])
                        {
                            clip.channels.push_back(std::move(tgtChannels[tb]));
                            ++retargetedChannels;
                        }

                    _inoutClips.push_back(std::move(clip));
                }

                if (_outDiag)
                {
                    _outDiag->aligned = aligned;
                    _outDiag->heightRatio = heightRatio;
                    _outDiag->limbSamples = diagSamples;
                    if (aligned)
                        _outDiag->srcUp = Bs.up;
                    if (diagSamples > 0)
                    {
                        _outDiag->maxLimbAngleDeg = diagAngleMax;
                        _outDiag->meanLimbAngleDeg =
                            static_cast<float>(diagAngleSum / diagSamples);
                    }
                    // 루트모션 순변위 = 마지막 − 처음 pos 키(방출된 그 트랙에서 직접). 축 결함(§5.8)
                    // 회귀 감시용 — 전진이 수평 성분에 실려야 한다.
                    if (emitRootMotion && !_inoutClips.empty())
                    {
                        const AnimClip& last = _inoutClips.back();
                        for (const AnimChannel& ch : last.channels)
                            if (ch.boneIndex == tgtRootMotionBone && ch.pos.size() >= 2)
                            {
                                _outDiag->rootMotionNet = ch.pos.back().value - ch.pos.front().value;
                                break;
                            }
                    }
                }

                MG_LOG_INFO("AssimpBaker: retargeted {} channel(s), {} unmatched, heightRatio {:.4f}",
                    retargetedChannels, unmatched, heightRatio);
                return unmatched;
            }

            // 타깃 스켈레톤 → 리타게팅 매칭 맵 준비 (BakeSkinned / RetargetAnims 공용).
            //   _outNormIdx: 정규화 본 이름 → 인덱스(동일 리그 exact 매칭).
            //   _outRoleIdx: 휴머노이드 역할 → 인덱스(첫 등장 우선, 크로스 리그 fallback).
            // (구: hips LOCAL 바인드 높이도 냈으나 루트모션 이동 스케일이 **각 리그 up 축 투영**
            //  기반으로 바뀌면서(RetargetAndExtractClips heightRatio) 죽은 값이 됐다. 그 자체가
            //  Y-up 가정이기도 해 제거했다.)
            void BuildRetargetTargetMaps(const Skeleton& _skeleton,
                std::unordered_map<std::string, int>& _outNormIdx,
                std::unordered_map<HumanoidBone, int>& _outRoleIdx)
            {
                _outNormIdx.clear();
                _outRoleIdx.clear();

                const size_t count = _skeleton.bones.size();
                for (size_t i = 0; i < count; ++i)
                {
                    const std::string norm = NormalizeBoneName(_skeleton.bones[i].name);
                    _outNormIdx.emplace(norm, static_cast<int>(i));
                    const HumanoidBone role = ResolveHumanoidBone(norm);
                    if (role != HumanoidBone::None)
                        _outRoleIdx.emplace(role, static_cast<int>(i)); // emplace = 첫 본 유지
                }
            }

            // 이미 임포트한 씬의 채널→타깃본 매칭을 표로 만든다(AnalyzeRetarget/ValidateRetarget 공용
            // — 씬을 두 번 임포트하지 않으려고 분리했다. 임포트가 이 경로의 지배적 비용).
            // 소스 채널 본 이름은 애니 전반에서 **중복 제거**한다(여러 클립이 같은 본 집합 공유).
            RetargetReport BuildRetargetReport(const aiScene* _scene,
                const Skeleton& _targetSkeleton,
                const std::unordered_map<std::string, int>& _normIdx,
                const std::unordered_map<HumanoidBone, int>& _roleIdx)
            {
                RetargetReport report;
                std::unordered_set<std::string> seen;
                for (unsigned int a = 0; a < _scene->mNumAnimations; ++a)
                {
                    const aiAnimation* anim = _scene->mAnimations[a];
                    for (unsigned int c = 0; c < anim->mNumChannels; ++c)
                    {
                        const std::string nodeName = anim->mChannels[c]->mNodeName.C_Str();
                        if (!seen.insert(nodeName).second)
                            continue; // 이미 본 소스 본

                        RetargetChannelMatch match;
                        match.sourceBone = nodeName;
                        match.targetBone = MatchChannelToTarget(nodeName, _normIdx, _roleIdx, match.kind);
                        if (match.targetBone >= 0)
                        {
                            match.targetBoneName = _targetSkeleton.bones[match.targetBone].name;
                            ++report.matched;
                        }
                        else
                        {
                            ++report.unmatched;
                        }
                        report.channels.push_back(std::move(match));
                    }
                }

                if (report.channels.empty())
                {
                    report.message = "no animation channels found in source";
                    return report;
                }
                report.success = true;
                report.message = std::to_string(report.matched) + " matched, "
                    + std::to_string(report.unmatched) + " unmatched";
                return report;
            }

            // BakeUpAxis → 애셋 전체에 곱할 추가 회전 N (row-vector: v·N).
            //   ZUp   : (0,0,1) → (0,1,0)  = R_x(-90°)
            //   NegZUp: (0,0,-1) → (0,1,0) = R_x(+90°)
            // Auto/YUp 는 항등 — assimp 가 이미 축을 변환했다는 전제(정상 FBX). 자세한 배경은
            // AssimpBaker.h 의 BakeUpAxis 주석 참조.
            Matrix UpAxisMatrix(BakeUpAxis _axis)
            {
                switch (_axis)
                {
                case BakeUpAxis::ZUp:    return Matrix::CreateRotationX(-DirectX::XM_PIDIV2);
                case BakeUpAxis::NegZUp: return Matrix::CreateRotationX(+DirectX::XM_PIDIV2);
                case BakeUpAxis::Auto:
                case BakeUpAxis::YUp:
                default:                 return Matrix();
                }
            }

            // BakeForwardAxis → up 을 세운 **뒤** 곱할 yaw 회전 (row-vector: v·N). 정면을 +Z 로 보낸다.
            //   NegZ: 180° / PosX: -90° / NegX: +90° / PosZ: 항등
            // 부호 주의: CreateRotationY(+90°) 는 **+Z → +X** 이므로, +X 정면을 +Z 로 되돌리려면 -90° 다.
            // (Auto 는 호출부가 MeasureForwardYaw 의 실측값으로 대체한다.)
            Matrix ForwardAxisMatrix(BakeForwardAxis _axis)
            {
                switch (_axis)
                {
                case BakeForwardAxis::NegZ: return Matrix::CreateRotationY(DirectX::XM_PI);
                case BakeForwardAxis::PosX: return Matrix::CreateRotationY(-DirectX::XM_PIDIV2);
                case BakeForwardAxis::NegX: return Matrix::CreateRotationY(+DirectX::XM_PIDIV2);
                case BakeForwardAxis::Auto:
                case BakeForwardAxis::PosZ:
                default:                    return Matrix();
                }
            }

            // 방향 벡터 → 사람이 읽는 문자열("character up=(0.00, 1.00, 0.00) => +Y-up").
            // _label 은 축의 역할 이름("up"/"forward") — 우세축 접미사에도 같은 단어를 쓴다.
            // 정규화 안 된 영벡터면 빈 문자열. (벡터를 이미 손에 쥔 호출부와 공용 — 유도 중복 방지.)
            std::string DescribeAxisVector(const char* _label, Vector3 _v)
            {
                if (_v.Length() < 1e-5f)
                    return {};
                _v.Normalize();
                const float ax[3] = { std::fabs(_v.x), std::fabs(_v.y), std::fabs(_v.z) };
                const int   dom = (ax[0] > ax[1] && ax[0] > ax[2]) ? 0 : (ax[1] > ax[2] ? 1 : 2);
                const char* names[3] = { "X", "Y", "Z" };
                const float comp[3] = { _v.x, _v.y, _v.z };
                char buf[160];
                std::snprintf(buf, sizeof(buf), "character %s=(%.2f, %.2f, %.2f) => %c%s-%s",
                    _label, _v.x, _v.y, _v.z, comp[dom] < 0.0f ? '-' : '+', names[dom], _label);
                return buf;
            }

            std::string DescribeUpVector(Vector3 _up) { return DescribeAxisVector("up", _up); }

            // rest 에서 실측한 캐릭터 up(hips→head) 을 사람이 읽는 문자열로. upAxis 오버라이드를
            // 눈으로 고르게 하는 진단값 — 측정 불가면 빈 문자열.
            std::string DescribeCharacterUp(const Skeleton& _skeleton,
                const std::unordered_map<HumanoidBone, int>& _roleIdx)
            {
                const auto hips = _roleIdx.find(HumanoidBone::Hips);
                const auto head = _roleIdx.find(HumanoidBone::Head);
                if (hips == _roleIdx.end() || head == _roleIdx.end())
                    return {};

                std::vector<Matrix> g(_skeleton.bones.size());
                for (size_t i = 0; i < _skeleton.bones.size(); ++i)
                {
                    const int p = _skeleton.bones[i].parentIndex;
                    g[i] = (p < 0) ? _skeleton.bones[i].localBindPose
                        : _skeleton.bones[i].localBindPose * g[p];
                }
                return DescribeUpVector(g[head->second].Translation()
                    - g[hips->second].Translation());
            }

            Matrix MeasureForwardYaw(const Skeleton& _skeleton,
                const std::unordered_map<HumanoidBone, int>& _roleIdx,
                const Matrix& _U, std::string& _outDetected)
            {
                _outDetected.clear();

                std::vector<Matrix> g(_skeleton.bones.size());
                for (size_t i = 0; i < _skeleton.bones.size(); ++i)
                {
                    const int p = _skeleton.bones[i].parentIndex;
                    g[i] = (p < 0) ? _skeleton.bones[i].localBindPose
                        : _skeleton.bones[i].localBindPose * g[p];
                }

                const HumanoidBone want[4] = { HumanoidBone::Hips, HumanoidBone::Head,
                                               HumanoidBone::LeftUpperLeg, HumanoidBone::RightUpperLeg };
                Vector3 pos[4];
                for (int i = 0; i < 4; ++i)
                {
                    const auto it = _roleIdx.find(want[i]);
                    if (it == _roleIdx.end() || it->second < 0
                        || it->second >= static_cast<int>(g.size()))
                    {
                        MG_LOG_WARN("AssimpBaker: forward normalize skipped — "
                            "missing Hips/Head/UpperLeg role bone");
                        return Matrix();
                    }
                    pos[i] = g[it->second].Translation();
                }

                const CharBasis basis = MakeCharBasis(pos[0], pos[1], pos[2], pos[3]);
                if (!basis.valid)
                {
                    MG_LOG_WARN("AssimpBaker: forward normalize skipped — degenerate rest basis");
                    return Matrix();
                }

                Vector3 f = Vector3::TransformNormal(basis.fwd, _U);
                _outDetected = DescribeAxisVector("forward", f);

                f.y = 0.0f; // 수평 성분만으로 yaw 를 정한다(pitch/roll 은 up 축 정규화의 몫)
                if (f.Length() < 1e-4f)
                {
                    MG_LOG_WARN("AssimpBaker: forward normalize skipped — forward is parallel to up ({})",
                        _outDetected);
                    return Matrix();
                }
                f.Normalize();

                // 가장 가까운 ±Z/±X 로 스냅 — 이미 +Z 인 애셋에 **정확히 항등**을 주기 위함이다.
                float   degrees = 0.0f;
                Vector3 snapped(0.0f, 0.0f, 1.0f);
                if (std::fabs(f.z) >= std::fabs(f.x))
                {
                    if (f.z < 0.0f) { degrees = 180.0f; snapped = Vector3(0.0f, 0.0f, -1.0f); }
                }
                else if (f.x > 0.0f) { degrees = -90.0f; snapped = Vector3(1.0f, 0.0f, 0.0f); }
                else { degrees = +90.0f; snapped = Vector3(-1.0f, 0.0f, 0.0f); }

                const float dot = (std::max)(-1.0f, (std::min)(1.0f, f.Dot(snapped)));
                const float residualDeg = ToDegrees(std::acos(dot));
                if (residualDeg > 15.0f)
                    MG_LOG_WARN("AssimpBaker: measured {} is {:.1f} deg off the nearest axis — "
                        "snapping to yaw {:.0f} deg anyway (override with forwardAxis if wrong)",
                        _outDetected, residualDeg, degrees);
                else
                    MG_LOG_INFO("AssimpBaker: pre-normalize {} => yaw {:.0f} deg to face +Z",
                        _outDetected, degrees);

                return Matrix::CreateRotationY(ToRadians(degrees));
            }

            // 축/공간 정규화 (베이크 후처리)
            void NormalizeSkeletonSpace(Skeleton& _skeleton, std::vector<AnimClip>& _clips, const Matrix& _N)
            {
                const int count = static_cast<int>(_skeleton.bones.size());
                const int rm = FindRootMotionBone(_skeleton);
                if (rm < 0)
                {
                    MG_LOG_WARN("AssimpBaker: axis normalize skipped — no root motion bone");
                    return;
                }

                // 1) 바인드 글로벌(노드 체인) — parentIndex < 자기 인덱스 보장.
                std::vector<Matrix> g(count);
                for (int i = 0; i < count; ++i)
                {
                    const int p = _skeleton.bones[i].parentIndex;
                    g[i] = (p < 0) ? _skeleton.bones[i].localBindPose
                        : _skeleton.bones[i].localBindPose * g[p];
                }

                // 2) rm 의 strict 조상 집합.
                std::vector<bool> isAncestor(count, false);
                for (int a = _skeleton.bones[rm].parentIndex; a >= 0; a = _skeleton.bones[a].parentIndex)
                    isAncestor[a] = true;

                // 3) 재작성 대상 = rm 자신 + 조상의 자식(옆가지 서브트리 루트). 곱할 상수는
                //    K[i] = G_old[parent(i)] · N (부모 없으면 N). 나머지 본은 부모를 통해 ·N 만 받는다.
                const auto needsRewrite = [&](int _i) {
                    if (_i == rm) return true;
                    const int p = _skeleton.bones[_i].parentIndex;
                    return p >= 0 && isAncestor[p];
                    };
                const auto foldMatrix = [&](int _i) {
                    const int p = _skeleton.bones[_i].parentIndex;
                    return (p < 0) ? _N : g[p] * _N;
                    };
                const auto isIdentity = [](const Matrix& _m) {
                    for (int r = 0; r < 4; ++r)
                        for (int c = 0; c < 4; ++c)
                            if (std::fabs(_m.m[r][c] - (r == c ? 1.0f : 0.0f)) > 1e-5f)
                                return false;
                    return true;
                    };
                // 접을 게 없으면 조기 반환 — 재샘플/재분해의 부동소수 왕복조차 피해 **산출물이
                // 구 베이커와 동일**함을 보장한다. 조건: _N 이 항등이고 모든 조상 로컬이 항등
                // (⇒ 모든 g[조상] 과 foldMatrix 가 항등 ⇒ 모든 대입이 제자리). Mixamo 리그가
                // 이 경우다 — 실측: XBot 은 조상이 RootNode 하나이고 그 바인드가 항등.
                bool ancestorsIdentity = true;
                for (int i = 0; i < count && ancestorsIdentity; ++i)
                    if (isAncestor[i])
                        ancestorsIdentity = isIdentity(_skeleton.bones[i].localBindPose);
                if (isIdentity(_N) && ancestorsIdentity)
                {
                    MG_LOG_INFO("AssimpBaker: axis normalize is a no-op (root motion bone \"{}\" is already "
                        "in mesh space)", _skeleton.bones[rm].name);
                    return;
                }

                // 4) 클립 재작성 (스켈레톤 변경 전). 조상 채널은 표현 불가 → 경고 후 드롭.
                for (AnimClip& clip : _clips)
                {
                    for (AnimChannel& ch : clip.channels)
                    {
                        if (ch.boneIndex < 0 || ch.boneIndex >= count)
                            continue;
                        if (isAncestor[ch.boneIndex])
                        {
                            MG_LOG_WARN("AssimpBaker: clip \"{}\" animates \"{}\" (ancestor of root motion "
                                "bone \"{}\") — dropped by axis normalize",
                                clip.name, _skeleton.bones[ch.boneIndex].name,
                                _skeleton.bones[rm].name);
                            ch.pos.clear(); ch.rot.clear(); ch.scale.clear();
                            continue;
                        }
                        if (!needsRewrite(ch.boneIndex))
                            continue;

                        // 키 시각 합집합에서 재샘플 → local·K 합성 → 재분해. 빈 트랙도 바인드
                        // fallback 으로 채워지므로 세 트랙 모두 materialize 해야 한다
                        // (바인드가 곧 바뀌므로 fallback 에 맡길 수 없다).
                        std::vector<float> times;
                        for (const VecKey& k : ch.pos)   times.push_back(k.time);
                        for (const QuatKey& k : ch.rot)  times.push_back(k.time);
                        for (const VecKey& k : ch.scale) times.push_back(k.time);
                        if (times.empty())
                            continue;
                        std::sort(times.begin(), times.end());
                        times.erase(std::unique(times.begin(), times.end()), times.end());

                        const Matrix K = foldMatrix(ch.boneIndex);
                        std::vector<VecKey>  newPos, newScale;
                        std::vector<QuatKey> newRot;
                        newPos.reserve(times.size());
                        newRot.reserve(times.size());
                        newScale.reserve(times.size());
                        for (const float t : times)
                        {
                            BoneTRS trs;
                            clip.SampleBoneTRSAtTick(ch.boneIndex, t, _skeleton, trs);
                            const Matrix local = Matrix::CreateScale(trs.scale)
                                * Matrix::CreateFromQuaternion(trs.rot)
                                * Matrix::CreateTranslation(trs.pos);
                            Vector3 s, p; Quaternion q;
                            if (!(local * K).Decompose(s, q, p))
                            {
                                MG_LOG_WARN("AssimpBaker: clip \"{}\" bone \"{}\": non-decomposable "
                                    "transform at tick {} — axis normalize may be inexact",
                                    clip.name, _skeleton.bones[ch.boneIndex].name, t);
                            }
                            q.Normalize();
                            newPos.push_back({ t, p });
                            newRot.push_back({ t, q });
                            newScale.push_back({ t, s });
                        }
                        ch.pos = std::move(newPos);
                        ch.rot = std::move(newRot);
                        ch.scale = std::move(newScale);
                    }
                }

                // 5) 스켈레톤 재작성. 조상은 final 을 invBind 로 흡수하고 local 을 항등화한다.
                std::vector<Matrix> newLocal(count);
                std::vector<Matrix> newInvBind(count);
                for (int i = 0; i < count; ++i)
                {
                    newLocal[i] = _skeleton.bones[i].localBindPose;
                    newInvBind[i] = _skeleton.bones[i].inverseBindPose;
                    if (isAncestor[i])
                    {
                        newLocal[i] = Matrix();
                        newInvBind[i] = _skeleton.bones[i].inverseBindPose * g[i] * _N;
                    }
                    else if (needsRewrite(i))
                    {
                        newLocal[i] = _skeleton.bones[i].localBindPose * foldMatrix(i);
                    }
                }
                for (int i = 0; i < count; ++i)
                {
                    _skeleton.bones[i].localBindPose = newLocal[i];
                    _skeleton.bones[i].inverseBindPose = newInvBind[i];
                }

                MG_LOG_INFO("AssimpBaker: axis normalized — folded {} ancestor(s) into root motion bone "
                    "\"{}\"", std::count(isAncestor.begin(), isAncestor.end(), true),
                    _skeleton.bones[rm].name);
            }

            // 스키닝 경로: 스켈레톤 + 스키닝 정점(mesh 로컬, 오프셋 행렬 규약) + AnimClip 추출.
            // _extraAnimSources 는 동일 스켈레톤 애니 FBX — 본 이름 매칭으로 클립 병합.
            BakeResult BakeSkinned(const aiScene* _scene,
                const std::wstring& _srcPath,
                const std::wstring& _outMiniPath,
                const std::vector<std::wstring>& _extraAnimSources,
                const BakeOptions& _options)
            {
                BakeResult result;
                result.skinned = true;

                // 1) 전체 메시의 aiBone → 이름/offset 수집 (스켈레톤 공유 가정).
                std::unordered_map<std::string, aiMatrix4x4> boneOffsets;
                for (unsigned int m = 0; m < _scene->mNumMeshes; ++m)
                {
                    const aiMesh* mesh = _scene->mMeshes[m];
                    for (unsigned int b = 0; b < mesh->mNumBones; ++b)
                        boneOffsets.emplace(mesh->mBones[b]->mName.C_Str(),
                            mesh->mBones[b]->mOffsetMatrix);
                }

                // 2) 스켈레톤 구축 (본 노드 + 조상, DFS 순서 = 부모<자기).
                std::unordered_set<const aiNode*> needed;
                MarkNeededNodes(_scene->mRootNode, boneOffsets, needed);

                Skeleton skeleton;
                std::unordered_map<std::string, int> indexByName;
                std::vector<Matrix> globalBind;
                BuildSkeleton(_scene->mRootNode, -1, needed, boneOffsets,
                    skeleton, indexByName, globalBind);

                constexpr size_t MAX_BAKE_BONES = 128; // 셰이더 b2 boneMatrices[128] 상한
                if (skeleton.bones.size() > MAX_BAKE_BONES)
                {
                    result.message = "bone count " + std::to_string(skeleton.bones.size())
                        + " exceeds MAX_BONES(128)";
                    MG_LOG_ERROR("AssimpBaker: {}", result.message);
                    return result;
                }

                // 3) 스키닝 정점/인덱스 (본 있는 메시만, mesh 로컬 그대로 병합 — 오프셋 행렬 규약).
                std::vector<MiniSkinnedVertex> verts;
                std::vector<uint32_t>          indices;
                unsigned int skippedMeshes = 0;

                for (unsigned int m = 0; m < _scene->mNumMeshes; ++m)
                {
                    const aiMesh* mesh = _scene->mMeshes[m];
                    if (!mesh->HasBones())
                    {
                        ++skippedMeshes;
                        continue;
                    }

                    const uint32_t base = static_cast<uint32_t>(verts.size());
                    for (unsigned int v = 0; v < mesh->mNumVertices; ++v)
                    {
                        MiniSkinnedVertex out = {};
                        out.position[0] = mesh->mVertices[v].x;
                        out.position[1] = mesh->mVertices[v].y;
                        out.position[2] = mesh->mVertices[v].z;
                        if (mesh->HasNormals())
                        {
                            out.normal[0] = mesh->mNormals[v].x;
                            out.normal[1] = mesh->mNormals[v].y;
                            out.normal[2] = mesh->mNormals[v].z;
                        }
                        else
                        {
                            out.normal[1] = 1.0f;
                        }
                        if (mesh->HasTextureCoords(0))
                        {
                            out.uv[0] = mesh->mTextureCoords[0][v].x;
                            out.uv[1] = mesh->mTextureCoords[0][v].y;
                        }
                        verts.push_back(out);
                    }

                    for (unsigned int b = 0; b < mesh->mNumBones; ++b)
                    {
                        const aiBone* bone = mesh->mBones[b];
                        const auto it = indexByName.find(bone->mName.C_Str());
                        if (it == indexByName.end())
                            continue; // 스켈레톤에 없는 본(비정상) — 스킵
                        const uint32_t boneIndex = static_cast<uint32_t>(it->second);
                        for (unsigned int w = 0; w < bone->mNumWeights; ++w)
                        {
                            const aiVertexWeight& vw = bone->mWeights[w];
                            AddVertexWeight(verts[base + vw.mVertexId], boneIndex, vw.mWeight);
                        }
                    }

                    for (unsigned int f = 0; f < mesh->mNumFaces; ++f)
                    {
                        const aiFace& face = mesh->mFaces[f];
                        if (face.mNumIndices != 3)
                            continue;
                        indices.push_back(base + face.mIndices[0]);
                        indices.push_back(base + face.mIndices[1]);
                        indices.push_back(base + face.mIndices[2]);
                    }
                }

                if (verts.empty() || indices.empty())
                {
                    result.message = "no skinned geometry found";
                    MG_LOG_ERROR("AssimpBaker: {}", result.message);
                    return result;
                }
                if (skippedMeshes > 0)
                    MG_LOG_WARN("AssimpBaker: skipped {} boneless mesh(es) in skinned scene", skippedMeshes);

                // 가중치 합=1 정규화 (합 0 이면 bone0 강체 바인딩).
                for (MiniSkinnedVertex& v : verts)
                {
                    const float sum = v.boneWeights[0] + v.boneWeights[1]
                        + v.boneWeights[2] + v.boneWeights[3];
                    if (sum > 1e-6f)
                    {
                        for (float& w : v.boneWeights)
                            w /= sum;
                    }
                    else
                    {
                        v.boneIndices[0] = 0;
                        v.boneWeights[0] = 1.0f;
                    }
                }

                // 4) 애니메이션 → AnimClip (채널은 스켈레톤에 있는 노드만).
                //    메인 소스 클립은 타깃 스켈레톤용이라 리타게팅 불필요(기존 경로).
                std::vector<AnimClip> clips;
                ExtractClips(_scene, indexByName, FileStemUtf8(_srcPath), clips);

                // 4-b) 추가 애니 소스 병합 — 타깃 스켈레톤으로 리타게팅(본 이름 매핑 + 바인드 보정, §9).
                //      정규화 이름 매핑과 타깃 hips 로컬 바인드 높이는 소스마다 재계산 불필요 → 1회 준비.
                //      정규화 이름 map(동일 리그 exact 매칭) + 휴머노이드 역할 map(크로스 리그 fallback).
                std::unordered_map<std::string, int>  normalizedIndexByName;
                std::unordered_map<HumanoidBone, int> roleIndex;
                BuildRetargetTargetMaps(skeleton, normalizedIndexByName, roleIndex);

                for (const std::wstring& animSrc : _extraAnimSources)
                {
                    Assimp::Importer animImporter; // importer 가 scene 을 소유 — 소스마다 새 인스턴스
                    const aiScene* animScene = ImportScene(animImporter, animSrc);
                    // 애니 전용 FBX 는 메시가 없어 INCOMPLETE 플래그가 설정될 수 있음 — 허용.
                    if (!animScene || !animScene->mRootNode)
                    {
                        MG_LOG_WARN("AssimpBaker: extra anim import failed ({}): {}",
                            ToUtf8(animSrc), animImporter.GetErrorString());
                        continue;
                    }

                    const size_t before = clips.size();
                    const unsigned int unmatched = RetargetAndExtractClips(
                        animScene, skeleton, normalizedIndexByName, roleIndex,
                        FileStemUtf8(animSrc), clips);
                    if (unmatched > 0)
                        MG_LOG_WARN("AssimpBaker: extra anim {}: {} channel(s) had no matching bone",
                            ToUtf8(animSrc), unmatched);
                    if (clips.size() == before)
                        MG_LOG_WARN("AssimpBaker: extra anim {} contained no animations",
                            ToUtf8(animSrc));
                }

                // 4-c) 축/공간 정규화 — **클립이 어느 경로로 생성됐든**(메인 소스 raw ExtractClips /
                //      추가 소스 리타겟) 확정된 스켈레톤+클립에 한 번만 적용해 하나의 공간으로 수렴시킨다.
                //      실측 up 은 **정규화 직전**에 재야 사용자가 오버라이드를 고를 수 있다.
                result.detectedUp = DescribeCharacterUp(skeleton, roleIndex);
                if (!result.detectedUp.empty())
                    MG_LOG_INFO("AssimpBaker: pre-normalize {}", result.detectedUp);

                //      정면(yaw)까지 세운다 — N = U · Y (row-vector: up 을 세운 **뒤** yaw).
                //      Auto 는 rest 실측(90° 스냅), 명시값은 "이 애셋의 정면은 이 축"이라는 단언.
                //      두 회전을 한 N 으로 합쳐 넘기므로 클립/바인드 재작성은 여전히 **1회**다.
                //      실측은 오버라이드일 때도 돌린다 — detectedForward 는 진단값이라 항상 채운다.
                const Matrix U = UpAxisMatrix(_options.upAxis);
                const Matrix measured = MeasureForwardYaw(skeleton, roleIndex, U, result.detectedForward);
                const bool   autoFwd = (_options.forwardAxis == BakeForwardAxis::Auto);
                if (!autoFwd)
                    MG_LOG_INFO("AssimpBaker: forward axis overridden by option — "
                        "measurement above is diagnostic only");
                NormalizeSkeletonSpace(skeleton, clips,
                    U * (autoFwd ? measured : ForwardAxisMatrix(_options.forwardAxis)));

                // 5) 직렬화 (증분 15 WriteSkinnedMesh 재사용 — 쓰기 단일 출처).
                if (!MiniLoader::WriteSkinnedMesh(_outMiniPath, verts, indices, skeleton, clips,
                    MINI_BAKE_AXIS_NORMALIZED
                    | MINI_BAKE_FORWARD_NORMALIZED))
                {
                    result.message = "failed to write .mini";
                    return result;
                }

                result.success = true;
                result.vertexCount = static_cast<uint32_t>(verts.size());
                result.indexCount = static_cast<uint32_t>(indices.size());
                result.boneCount = static_cast<uint32_t>(skeleton.bones.size());
                result.clipCount = static_cast<uint32_t>(clips.size());
                result.message = "ok";
                MG_LOG_INFO("AssimpBaker: baked skinned {} verts, {} indices, {} bones, {} clips",
                    result.vertexCount, result.indexCount, result.boneCount, result.clipCount);
                return result;
            }
        }

        BakeResult AssimpBaker::Bake(const std::wstring& _srcPath,
            const std::wstring& _outMiniPath,
            const std::vector<std::wstring>& _extraAnimSources,
            const BakeOptions& _options)
        {
            BakeResult result;

            Assimp::Importer importer;
            const aiScene* scene = ImportScene(importer, _srcPath);
            if (!scene || (scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE) || !scene->mRootNode)
            {
                result.message = std::string("import failed: ") + importer.GetErrorString();
                MG_LOG_ERROR("AssimpBaker: {}", result.message);
                return result;
            }

            // 단위 확인(진단용): FBX 메타데이터의 UnitScaleFactor(cm/unit)를 로깅.
            // GlobalScale 스텝이 이미 정점을 미터로 변환한 뒤이므로 정보성 로그일 뿐이다
            // (1.0=cm 저작 → ×0.01 적용됨, 100.0=m 저작 → 무변환). 메타데이터 없으면 스킵.
            // assimp 는 UnitScaleFactor 를 float 로 저장 → aiMetadata::Get 은 타입 정합 필요(float).
            if (scene->mMetaData)
            {
                float unitScale = 1.0f;
                if (scene->mMetaData->Get(std::string("UnitScaleFactor"), unitScale))
                    MG_LOG_INFO("AssimpBaker: FBX UnitScaleFactor={} cm/unit (→ scaled to meters)",
                        unitScale);
            }

            // 자동 감지: 본 있는 메시가 하나라도 있으면 SkinnedMesh 로 베이크.
            bool hasBones = false;
            for (unsigned int m = 0; m < scene->mNumMeshes; ++m)
                hasBones |= scene->mMeshes[m]->HasBones();
            if (hasBones)
                return BakeSkinned(scene, _srcPath, _outMiniPath, _extraAnimSources, _options);

            // ── StaticMesh 경로 (기존 동작) ──
            if (!_extraAnimSources.empty())
                MG_LOG_WARN("AssimpBaker: extra anim sources ignored for static mesh ({})",
                    ToUtf8(_srcPath));
            std::vector<MiniStaticVertex> verts;
            std::vector<uint32_t>         indices;
            ProcessNode(scene->mRootNode, aiMatrix4x4(), scene, verts, indices);

            if (verts.empty() || indices.empty())
            {
                result.message = "no triangulated geometry found";
                MG_LOG_ERROR("AssimpBaker: {}", result.message);
                return result;
            }

            if (!MiniLoader::WriteStaticMesh(_outMiniPath, verts, indices))
            {
                result.message = "failed to write .mini";
                return result;
            }

            result.success = true;
            result.vertexCount = static_cast<uint32_t>(verts.size());
            result.indexCount = static_cast<uint32_t>(indices.size());
            result.message = "ok";
            MG_LOG_INFO("AssimpBaker: baked {} verts, {} indices",
                result.vertexCount, result.indexCount);
            return result;
        }

        BakeResult AssimpBaker::RetargetAnims(const Skeleton& _targetSkeleton,
            const std::vector<std::wstring>& _animSources,
            std::vector<AnimClip>& _inoutClips,
            const BakeOptions& _options)
        {
            BakeResult result;
            result.skinned = true;
            result.boneCount = static_cast<uint32_t>(_targetSkeleton.bones.size());
            result.clipCount = static_cast<uint32_t>(_inoutClips.size());

            if (_targetSkeleton.bones.empty())
            {
                result.message = "target skeleton is empty";
                MG_LOG_ERROR("AssimpBaker::RetargetAnims: {}", result.message);
                return result;
            }
            if (_animSources.empty())
            {
                result.message = "no animation sources";
                MG_LOG_WARN("AssimpBaker::RetargetAnims: {}", result.message);
                return result;
            }

            // Up Axis 오버라이드 → 소스에 적용할 월드 회전. Auto/YUp = 항등(무영향).
            const Matrix srcWorldRot = UpAxisMatrix(_options.upAxis);

            // 타깃 매칭 맵 준비 (BakeSkinned 와 공용 헬퍼 — 정규화 이름/역할 맵).
            std::unordered_map<std::string, int>  normalizedIndexByName;
            std::unordered_map<HumanoidBone, int> roleIndex;
            BuildRetargetTargetMaps(_targetSkeleton, normalizedIndexByName, roleIndex);

            unsigned int addedClips = 0;
            unsigned int totalUnmatched = 0;
            for (const std::wstring& animSrc : _animSources)
            {
                Assimp::Importer animImporter; // importer 가 scene 을 소유 — 소스마다 새 인스턴스
                const aiScene* animScene = ImportScene(animImporter, animSrc);
                // 애니 전용 FBX 는 메시가 없어 INCOMPLETE 플래그가 설정될 수 있음 — 허용(루트만 확인).
                if (!animScene || !animScene->mRootNode)
                {
                    MG_LOG_WARN("AssimpBaker::RetargetAnims: import failed ({}): {}",
                        ToUtf8(animSrc), animImporter.GetErrorString());
                    continue;
                }

                const size_t before = _inoutClips.size();
                const unsigned int unmatched = RetargetAndExtractClips(
                    animScene, _targetSkeleton, normalizedIndexByName, roleIndex,
                    FileStemUtf8(animSrc), _inoutClips, nullptr, nullptr, srcWorldRot);
                totalUnmatched += unmatched;
                addedClips += static_cast<unsigned int>(_inoutClips.size() - before);
                if (unmatched > 0)
                    MG_LOG_WARN("AssimpBaker::RetargetAnims: {}: {} channel(s) had no matching bone",
                        ToUtf8(animSrc), unmatched);
                if (_inoutClips.size() == before)
                    MG_LOG_WARN("AssimpBaker::RetargetAnims: {} contained no animations",
                        ToUtf8(animSrc));
            }

            result.clipCount = static_cast<uint32_t>(_inoutClips.size());
            if (addedClips == 0)
            {
                result.message = "no clips added (no animations found in sources)";
                MG_LOG_WARN("AssimpBaker::RetargetAnims: {}", result.message);
                return result;
            }

            result.success = true;
            result.message = "added " + std::to_string(addedClips) + " clip(s)";
            if (totalUnmatched > 0)
                result.message += " (" + std::to_string(totalUnmatched) + " unmatched channel(s))";
            MG_LOG_INFO("AssimpBaker::RetargetAnims: added {} clip(s), {} unmatched, {} total clips",
                addedClips, totalUnmatched, result.clipCount);
            return result;
        }

        RetargetReport AssimpBaker::AnalyzeRetarget(const Skeleton& _targetSkeleton,
            const std::wstring& _animSource)
        {
            RetargetReport report;
            if (_targetSkeleton.bones.empty())
            {
                report.message = "target skeleton is empty";
                return report;
            }

            // 타깃 매칭 맵(정규화 이름/역할) — RetargetAnims 와 공용 헬퍼.
            std::unordered_map<std::string, int>  normalizedIndexByName;
            std::unordered_map<HumanoidBone, int> roleIndex;
            BuildRetargetTargetMaps(_targetSkeleton, normalizedIndexByName, roleIndex);

            Assimp::Importer importer;
            const aiScene* scene = ImportScene(importer, _animSource);
            if (!scene || !scene->mRootNode) // 애니 전용 FBX 는 메시 없어 INCOMPLETE 가능 — 루트만 확인
            {
                report.message = "import failed: " + std::string(importer.GetErrorString());
                MG_LOG_WARN("AssimpBaker::AnalyzeRetarget: {} ({})", report.message, ToUtf8(_animSource));
                return report;
            }

            report = BuildRetargetReport(scene, _targetSkeleton, normalizedIndexByName, roleIndex);
            if (!report.success)
            {
                MG_LOG_WARN("AssimpBaker::AnalyzeRetarget: {} ({})", report.message, ToUtf8(_animSource));
                return report;
            }
            MG_LOG_INFO("AssimpBaker::AnalyzeRetarget: {} channel(s) ({})",
                report.channels.size(), report.message);
            return report;
        }

        BakeResult AssimpBaker::RetargetAnims(const Skeleton& _targetSkeleton,
            const std::wstring& _animSource,
            const std::unordered_map<std::string, int>& _overrides,
            std::vector<AnimClip>& _inoutClips,
            const BakeOptions& _options)
        {
            BakeResult result;
            result.skinned = true;
            result.boneCount = static_cast<uint32_t>(_targetSkeleton.bones.size());
            result.clipCount = static_cast<uint32_t>(_inoutClips.size());

            if (_targetSkeleton.bones.empty())
            {
                result.message = "target skeleton is empty";
                MG_LOG_ERROR("AssimpBaker::RetargetAnims(override): {}", result.message);
                return result;
            }

            // Up Axis 오버라이드 → 소스에 적용할 월드 회전. Auto/YUp = 항등(무영향).
            const Matrix srcWorldRot = UpAxisMatrix(_options.upAxis);

            std::unordered_map<std::string, int>  normalizedIndexByName;
            std::unordered_map<HumanoidBone, int> roleIndex;
            BuildRetargetTargetMaps(_targetSkeleton, normalizedIndexByName, roleIndex);

            Assimp::Importer animImporter;
            const aiScene* animScene = ImportScene(animImporter, _animSource);
            if (!animScene || !animScene->mRootNode)
            {
                result.message = "import failed: " + std::string(animImporter.GetErrorString());
                MG_LOG_WARN("AssimpBaker::RetargetAnims(override): {} ({})",
                    result.message, ToUtf8(_animSource));
                return result;
            }

            const size_t before = _inoutClips.size();
            const unsigned int unmatched = RetargetAndExtractClips(
                animScene, _targetSkeleton, normalizedIndexByName, roleIndex,
                FileStemUtf8(_animSource), _inoutClips, &_overrides, nullptr, srcWorldRot);

            const unsigned int addedClips = static_cast<unsigned int>(_inoutClips.size() - before);
            result.clipCount = static_cast<uint32_t>(_inoutClips.size());
            if (addedClips == 0)
            {
                result.message = "no clips added (no animations found in source)";
                MG_LOG_WARN("AssimpBaker::RetargetAnims(override): {}", result.message);
                return result;
            }

            result.success = true;
            result.message = "added " + std::to_string(addedClips) + " clip(s)";
            if (unmatched > 0)
                result.message += " (" + std::to_string(unmatched) + " unmatched channel(s))";
            MG_LOG_INFO("AssimpBaker::RetargetAnims(override): added {} clip(s), {} unmatched, {} total clips",
                addedClips, unmatched, result.clipCount);
            return result;
        }

        RetargetValidation AssimpBaker::ValidateRetarget(const Skeleton& _targetSkeleton,
            const std::wstring& _animSource)
        {
            RetargetValidation v;
            if (_targetSkeleton.bones.empty())
            {
                v.message = "target skeleton is empty";
                MG_LOG_ERROR("AssimpBaker::ValidateRetarget: {}", v.message);
                return v;
            }

            std::unordered_map<std::string, int>  normalizedIndexByName;
            std::unordered_map<HumanoidBone, int> roleIndex;
            BuildRetargetTargetMaps(_targetSkeleton, normalizedIndexByName, roleIndex);

            Assimp::Importer importer;
            const aiScene* scene = ImportScene(importer, _animSource);
            if (!scene || !scene->mRootNode) // 애니 전용 FBX 는 메시 없어 INCOMPLETE 가능 — 루트만 확인
            {
                v.message = "import failed: " + std::string(importer.GetErrorString());
                MG_LOG_WARN("AssimpBaker::ValidateRetarget: {} ({})", v.message, ToUtf8(_animSource));
                return v;
            }

            v.report = BuildRetargetReport(scene, _targetSkeleton, normalizedIndexByName, roleIndex);
            if (!v.report.success)
            {
                v.message = v.report.message;
                MG_LOG_WARN("AssimpBaker::ValidateRetarget: {} ({})", v.message, ToUtf8(_animSource));
                return v;
            }

            // **실제 베이크 경로**로 리타게팅해 지표를 받는다. 클립은 로컬에 버린다(파일 미기록).
            std::vector<AnimClip> clips;
            RetargetDiag diag;
            RetargetAndExtractClips(scene, _targetSkeleton, normalizedIndexByName, roleIndex,
                FileStemUtf8(_animSource), clips, nullptr, &diag);
            if (clips.empty())
            {
                v.message = "source contained no animations";
                MG_LOG_WARN("AssimpBaker::ValidateRetarget: {} ({})", v.message, ToUtf8(_animSource));
                return v;
            }

            v.success = true;
            v.clipCount = static_cast<uint32_t>(clips.size());
            v.clipName = clips.front().name;
            v.durationSec = (clips.front().ticksPerSecond > 0.0f)
                ? clips.front().duration / clips.front().ticksPerSecond : 0.0f;
            v.aligned = diag.aligned;
            v.heightRatio = diag.heightRatio;
            v.maxLimbAngleDeg = diag.maxLimbAngleDeg;
            v.meanLimbAngleDeg = diag.meanLimbAngleDeg;
            v.limbSamples = diag.limbSamples;
            v.detectedUp = DescribeUpVector(diag.srcUp);
            v.rootMotionNet[0] = diag.rootMotionNet.x;
            v.rootMotionNet[1] = diag.rootMotionNet.y;
            v.rootMotionNet[2] = diag.rootMotionNet.z;
            v.message = v.report.message;
            if (!v.aligned)
                v.message += " | NOT ALIGNED (rig basis unavailable)";
            else if (v.limbSamples > 0)
            {
                char buf[96];
                std::snprintf(buf, sizeof(buf), " | limb max %.3f° mean %.3f°",
                    v.maxLimbAngleDeg, v.meanLimbAngleDeg);
                v.message += buf;
            }

            MG_LOG_INFO("AssimpBaker::ValidateRetarget: {} — clip \"{}\" {:.2f}s, {}, "
                "aligned={}, heightRatio={:.4f}, limb max={:.3f}° mean={:.3f}° ({} samples), "
                "rootMotionNet=({:.3f}, {:.3f}, {:.3f})",
                ToUtf8(_animSource), v.clipName, v.durationSec, v.report.message,
                v.aligned, v.heightRatio, v.maxLimbAngleDeg, v.meanLimbAngleDeg,
                v.limbSamples, v.rootMotionNet[0], v.rootMotionNet[1], v.rootMotionNet[2]);
            return v;
        }
    }
}

#else
// ─── Debug/Release 구성: no-op 스텁 (assimp 심볼 미참조) ──────────────────────
namespace MiniEngine
{
    namespace Editor
    {
        BakeResult AssimpBaker::Bake(const std::wstring&, const std::wstring&,
            const std::vector<std::wstring>&, const BakeOptions&)
        {
            BakeResult result;
            result.message = "Baker unavailable (Editor only)";
            return result;
        }

        BakeResult AssimpBaker::RetargetAnims(const Skeleton&,
            const std::vector<std::wstring>&,
            std::vector<AnimClip>&,
            const BakeOptions&)
        {
            BakeResult result;
            result.message = "Baker unavailable (Editor only)";
            return result; // _inoutClips 무변경
        }

        RetargetReport AssimpBaker::AnalyzeRetarget(const Skeleton&, const std::wstring&)
        {
            RetargetReport report;
            report.message = "Baker unavailable (Editor only)";
            return report;
        }

        BakeResult AssimpBaker::RetargetAnims(const Skeleton&, const std::wstring&,
            const std::unordered_map<std::string, int>&,
            std::vector<AnimClip>&,
            const BakeOptions&)
        {
            BakeResult result;
            result.message = "Baker unavailable (Editor only)";
            return result; // _inoutClips 무변경
        }

        RetargetValidation AssimpBaker::ValidateRetarget(const Skeleton&, const std::wstring&)
        {
            RetargetValidation v;
            v.message = "Baker unavailable (Editor only)";
            return v;
        }
    }
}
#endif
