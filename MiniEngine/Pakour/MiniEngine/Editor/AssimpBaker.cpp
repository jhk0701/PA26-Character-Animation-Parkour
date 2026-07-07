#include "pch.h"
#include "Editor/AssimpBaker.h"

#if defined(WITH_EDITOR)
// ─── Editor 구성: 실제 Assimp 구현 ──────────────────────────────────────────
#include "Asset/MiniFormat.h"
#include "Asset/MiniLoader.h" // WriteStaticMesh/WriteSkinnedMesh (+Skeleton/AnimClip 타입)
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
#include <cstdint>
#include <cmath>
#include <cctype> // std::tolower (본 이름 정규화)
#include <windows.h> // WideCharToMultiByte (wstring → UTF-8 경로)

namespace MiniEngine::Editor
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

        // 클립 이름 규칙: 비었거나 기존 목록과 중복이면 _fallbackStem, 그래도 중복이면
        // "_N" 접미사(Mixamo 애니가 전부 "mixamo.com" 인 중복 대응 — UI 구분용).
        std::string ResolveClipName(std::string _name, const std::string& _fallbackStem,
            const std::vector<AnimClip>& _clips)
        {
            const auto taken = [&_clips](const std::string& _n) {
                for (const AnimClip& c : _clips)
                    if (c.name == _n)
                        return true;
                return false;
            };

            if (_name.empty() || taken(_name))
                _name = _fallbackStem.empty() ? "clip" : _fallbackStem;
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

        // 본 이름 정규화: 네임스페이스(마지막 ':' 이전) 제거 + 소문자화.
        // "mixamorig:Hips" → "hips". 서로 다른 리그의 접두사 차이를 흡수(본 이름 매핑).
        std::string NormalizeBoneName(std::string _name)
        {
            const size_t colon = _name.find_last_of(':');
            if (colon != std::string::npos)
                _name = _name.substr(colon + 1);
            for (char& ch : _name)
                ch = static_cast<char>(std::tolower(static_cast<unsigned char>(ch)));
            return _name;
        }

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

        // 추가 애니 소스를 타깃 스켈레톤으로 "리타게팅"해 클립으로 추출·병합.
        //  - 채널→본 매칭: 정규화 이름으로 타깃 본 인덱스 탐색(미매칭 수 반환).
        //  - 회전: 바인드 델타 보정 q_out = qBindTgt * Inverse(qBindSrc) * q_key (row-vector 규약).
        //          소스·타깃 바인드가 같으면 항등(동일 리그 회귀 안전).
        //  - 트랜슬레이션: 루트 모션 본(정규화 이름 "hips")만 높이 비율로 스케일해 방출, 나머지는
        //          방출 안 함 → 런타임이 타깃 바인드 성분으로 fallback(비율 늘어짐 제거). scale 은 드롭.
        unsigned int RetargetAndExtractClips(
            const aiScene* _animScene,
            const Skeleton& _targetSkeleton,
            const std::unordered_map<std::string, int>& _normalizedTargetIndex,
            float _targetHipsGlobalY,
            const std::string& _fallbackStem,
            std::vector<AnimClip>& _inoutClips)
        {
            // 1) 소스 rest 포즈 수집 + 소스 hips 글로벌 높이 → 루트 모션 스케일 비율.
            std::unordered_map<std::string, Matrix> srcLocal, srcGlobal;
            CollectRestPose(_animScene->mRootNode, Matrix(), srcLocal, srcGlobal);

            float srcHipsGlobalY = 0.0f;
            for (const auto& kv : srcGlobal)
                if (NormalizeBoneName(kv.first) == "hips")
                {
                    srcHipsGlobalY = kv.second.Translation().y; break;
                }

            const float heightRatio =
                (std::fabs(srcHipsGlobalY) > 1e-4f && std::fabs(_targetHipsGlobalY) > 1e-4f)
                ? (_targetHipsGlobalY / srcHipsGlobalY) : 1.0f;

            unsigned int unmatched = 0;
            unsigned int retargetedChannels = 0;

            for (unsigned int a = 0; a < _animScene->mNumAnimations; ++a)
            {
                const aiAnimation* anim = _animScene->mAnimations[a];

                AnimClip clip;
                clip.name = ResolveClipName(anim->mName.C_Str(), _fallbackStem, _inoutClips);
                clip.duration = static_cast<float>(anim->mDuration);
                clip.ticksPerSecond = (anim->mTicksPerSecond != 0.0)
                    ? static_cast<float>(anim->mTicksPerSecond) : 25.0f;

                for (unsigned int c = 0; c < anim->mNumChannels; ++c)
                {
                    const aiNodeAnim* nodeAnim = anim->mChannels[c];
                    const std::string nodeName = nodeAnim->mNodeName.C_Str();
                    const std::string norm = NormalizeBoneName(nodeName);

                    const auto it = _normalizedTargetIndex.find(norm);
                    if (it == _normalizedTargetIndex.end())
                    {
                        ++unmatched; // 타깃 스켈레톤에 없는 본(카메라/여분 노드 등)
                        continue;
                    }
                    const int  targetBoneIndex = it->second;
                    const bool isRoot = (norm == "hips");

                    // 소스/타깃 바인드 로컬 회전 → 보정 쿼터니언 qFix.
                    Quaternion srcRot, tgtRot;
                    {
                        Vector3 s, t; // 스케일/트랜슬레이션 미사용
                        const auto srcIt = srcLocal.find(nodeName);
                        Matrix srcMat = (srcIt != srcLocal.end()) ? srcIt->second : Matrix();
                        srcMat.Decompose(s, srcRot, t);
                        Matrix tgtMat = _targetSkeleton.bones[targetBoneIndex].localBindPose;
                        tgtMat.Decompose(s, tgtRot, t);
                    }
                    Quaternion invSrc;
                    srcRot.Conjugate(invSrc);       // 단위 쿼터니언 → 켤레 = 역
                    const Quaternion qFix = tgtRot * invSrc;

                    AnimChannel channel;
                    channel.boneIndex = targetBoneIndex;

                    channel.rot.reserve(nodeAnim->mNumRotationKeys);
                    for (unsigned int k = 0; k < nodeAnim->mNumRotationKeys; ++k)
                    {
                        const aiQuatKey& key = nodeAnim->mRotationKeys[k];
                        const Quaternion qKey(key.mValue.x, key.mValue.y, key.mValue.z, key.mValue.w);
                        Quaternion qOut = qFix * qKey; // qBindTgt * Inverse(qBindSrc) * q_key
                        qOut.Normalize();
                        channel.rot.push_back({ static_cast<float>(key.mTime), qOut });
                    }

                    // 루트 모션 본만 pos 방출(높이 비율 스케일). 나머지는 방출 안 함
                    // → 런타임 SampleTRS 가 타깃 바인드 트랜슬레이션으로 fallback.
                    if (isRoot)
                    {
                        channel.pos.reserve(nodeAnim->mNumPositionKeys);
                        for (unsigned int k = 0; k < nodeAnim->mNumPositionKeys; ++k)
                        {
                            const aiVectorKey& key = nodeAnim->mPositionKeys[k];
                            channel.pos.push_back({ static_cast<float>(key.mTime),
                                Vector3(key.mValue.x * heightRatio,
                                        key.mValue.y * heightRatio,
                                        key.mValue.z * heightRatio) });
                        }
                    }

                    clip.channels.push_back(std::move(channel));
                    ++retargetedChannels;
                }

                _inoutClips.push_back(std::move(clip));
            }

            MG_LOG_INFO("AssimpBaker: retargeted {} channel(s), {} unmatched, heightRatio {:.4f}",
                retargetedChannels, unmatched, heightRatio);
            return unmatched;
        }

        // 스키닝 경로: 스켈레톤 + 스키닝 정점(mesh 로컬, 오프셋 행렬 규약) + AnimClip 추출.
        // _extraAnimSources 는 동일 스켈레톤 애니 FBX — 본 이름 매칭으로 클립 병합.
        BakeResult BakeSkinned(const aiScene* _scene,
            const std::wstring& _srcPath,
            const std::wstring& _outMiniPath,
            const std::vector<std::wstring>& _extraAnimSources)
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
            //      정규화 이름 매핑과 타깃 hips 글로벌 바인드 높이는 소스마다 재계산 불필요 → 1회 준비.
            std::unordered_map<std::string, int> normalizedIndexByName;
            for (size_t i = 0; i < skeleton.bones.size(); ++i)
                normalizedIndexByName.emplace(NormalizeBoneName(skeleton.bones[i].name),
                    static_cast<int>(i));

            float targetHipsGlobalY = 0.0f;
            for (size_t i = 0; i < skeleton.bones.size(); ++i)
                if (NormalizeBoneName(skeleton.bones[i].name) == "hips")
                {
                    targetHipsGlobalY = globalBind[i].Translation().y; break;
                }

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
                    animScene, skeleton, normalizedIndexByName, targetHipsGlobalY,
                    FileStemUtf8(animSrc), clips);
                if (unmatched > 0)
                    MG_LOG_WARN("AssimpBaker: extra anim {}: {} channel(s) had no matching bone",
                        ToUtf8(animSrc), unmatched);
                if (clips.size() == before)
                    MG_LOG_WARN("AssimpBaker: extra anim {} contained no animations",
                        ToUtf8(animSrc));
            }

            // 5) 직렬화 (증분 15 WriteSkinnedMesh 재사용 — 쓰기 단일 출처).
            if (!MiniLoader::WriteSkinnedMesh(_outMiniPath, verts, indices, skeleton, clips))
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
        const std::vector<std::wstring>& _extraAnimSources)
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

        // 단위 확인
        if (scene->mMetaData) 
        {
            float unitScale = 1.0f;
            if (scene->mMetaData->Get(std::string("UnitScaleFactor"), unitScale))
                MG_LOG_INFO("AssimpBaker: FBX UnitScaleFactor={} cm/unit (→ scaled to meters)", unitScale);
        }

        // 자동 감지: 본 있는 메시가 하나라도 있으면 SkinnedMesh 로 베이크.
        bool hasBones = false;
        for (unsigned int m = 0; m < scene->mNumMeshes; ++m)
            hasBones |= scene->mMeshes[m]->HasBones();
        if (hasBones)
            return BakeSkinned(scene, _srcPath, _outMiniPath, _extraAnimSources);

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
}

#else
// ─── Debug/Release 구성: no-op 스텁 (assimp 심볼 미참조) ──────────────────────
namespace MiniEngine
{
    namespace Editor
    {
        BakeResult AssimpBaker::Bake(const std::wstring&, const std::wstring&,
                                     const std::vector<std::wstring>&)
        {
            BakeResult result;
            result.message = "Baker unavailable (Editor only)";
            return result;
        }
    }
}
#endif
