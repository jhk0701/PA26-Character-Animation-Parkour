#include "pch.h"
#include "Asset/MiniLoader.h"
#include "Asset/MiniFormat.h"
#include "Core/Log.h"
#include "Core/Math.h"
#include <fstream>
#include <cstdint>
#include <cstring>
#include <filesystem>

namespace MiniEngine
{
    namespace
    {
        // 단위 큐브(-1..1). 면당 4정점 * 6면 = 24정점, 면별 법선(외향). LH / CW(외부 시점).
        // 각 면: v0(BL) v1(BR) v2(TR) v3(TL) 순서, 삼각형 (0,2,1) (0,3,2) — CW 외향.
        struct CubeFace
        {
            float normal[3];
            float corners[4][3];
        };

        const CubeFace kCubeFaces[6] =
        {
            // +Z (front)
            { { 0, 0, 1 },  { { -1, -1, 1 }, { 1, -1, 1 }, { 1, 1, 1 }, { -1, 1, 1 } } },
            // -Z (back)
            { { 0, 0, -1 }, { { 1, -1, -1 }, { -1, -1, -1 }, { -1, 1, -1 }, { 1, 1, -1 } } },
            // +X (right)
            { { 1, 0, 0 },  { { 1, -1, 1 }, { 1, -1, -1 }, { 1, 1, -1 }, { 1, 1, 1 } } },
            // -X (left)
            { { -1, 0, 0 }, { { -1, -1, -1 }, { -1, -1, 1 }, { -1, 1, 1 }, { -1, 1, -1 } } },
            // +Y (top)
            { { 0, 1, 0 },  { { -1, 1, 1 }, { 1, 1, 1 }, { 1, 1, -1 }, { -1, 1, -1 } } },
            // -Y (bottom)
            { { 0, -1, 0 }, { { -1, -1, -1 }, { 1, -1, -1 }, { 1, -1, 1 }, { -1, -1, 1 } } },
        };

        const float kCornerUV[4][2] = { { 0, 0 }, { 1, 0 }, { 1, 1 }, { 0, 1 } };

        void BuildCube(std::vector<MiniStaticVertex>& _vertices, std::vector<uint32_t>& _indices)
        {
            _vertices.clear();
            _indices.clear();
            _vertices.reserve(24);
            _indices.reserve(36);

            for (const CubeFace& face : kCubeFaces)
            {
                const uint32_t base = static_cast<uint32_t>(_vertices.size());
                for (int i = 0; i < 4; ++i)
                {
                    MiniStaticVertex v = {};
                    v.position[0] = face.corners[i][0];
                    v.position[1] = face.corners[i][1];
                    v.position[2] = face.corners[i][2];
                    v.normal[0]   = face.normal[0];
                    v.normal[1]   = face.normal[1];
                    v.normal[2]   = face.normal[2];
                    v.uv[0]       = kCornerUV[i][0];
                    v.uv[1]       = kCornerUV[i][1];
                    _vertices.push_back(v);
                }
                // 삼각형 (0,2,1) (0,3,2) — CW 외향(LH).
                _indices.push_back(base + 0);
                _indices.push_back(base + 2);
                _indices.push_back(base + 1);
                _indices.push_back(base + 0);
                _indices.push_back(base + 3);
                _indices.push_back(base + 2);
            }
        }

        // ---- SkinnedMesh 직렬화 헬퍼 ----

        // SimpleMath Matrix(row-major 저장) ↔ float[16] (레이아웃 동일 — memcpy).
        void MatrixToFloats(const Matrix& _m, float (&_out)[16])
        {
            std::memcpy(_out, &_m, sizeof(float) * 16);
        }

        Matrix FloatsToMatrix(const float (&_in)[16])
        {
            Matrix m;
            std::memcpy(&m, _in, sizeof(float) * 16);
            return m;
        }

        void CopyName(char (&_dst)[MINI_NAME_LENGTH], const std::string& _src)
        {
            std::memset(_dst, 0, MINI_NAME_LENGTH);
            const size_t len = (_src.size() < MINI_NAME_LENGTH - 1) ? _src.size() : MINI_NAME_LENGTH - 1;
            std::memcpy(_dst, _src.data(), len);
        }

        // 널 종단 보장 읽기(파일 손상 대비).
        std::string ReadName(const char (&_src)[MINI_NAME_LENGTH])
        {
            const size_t len = ::strnlen(_src, MINI_NAME_LENGTH);
            return std::string(_src, len);
        }

        // ---- 절차적 2-본 굴곡 박스 (검증용, Assimp 미사용) ----
        //
        // 1×4×1 박스(x,z ∈ [-0.5,0.5], y ∈ [0,4])를 Y축으로 8링 분할.
        // bone0(root, y=0 바인드 identity) / bone1(자식, y=2 관절).
        // 정점 가중치 = 높이 선형 블렌드(y=0 → bone0 100%, y=4 → bone1 100%).
        void BuildSkinnedTestBox(std::vector<MiniSkinnedVertex>& _vertices, std::vector<uint32_t>& _indices)
        {
            constexpr float HALF      = 0.5f;
            constexpr float HEIGHT    = 4.0f;
            constexpr int   SEGMENTS  = 8;
            constexpr float SEG_DY    = HEIGHT / SEGMENTS;

            _vertices.clear();
            _indices.clear();

            // 가중치는 높이 비례 — 하단 bone0, 상단 bone1 로 선형 블렌드.
            auto pushVertex = [&_vertices, &HEIGHT](float _x, float _y, float _z,
                                           float _nx, float _ny, float _nz, float _u, float _v)
            {
                MiniSkinnedVertex vert = {};
                vert.position[0] = _x;  vert.position[1] = _y;  vert.position[2] = _z;
                vert.normal[0]   = _nx; vert.normal[1]   = _ny; vert.normal[2]   = _nz;
                vert.uv[0] = _u; vert.uv[1] = _v;
                const float w1 = _y / HEIGHT;
                vert.boneIndices[0] = 0;
                vert.boneIndices[1] = 1;
                vert.boneWeights[0] = 1.0f - w1;
                vert.boneWeights[1] = w1;
                _vertices.push_back(vert);
            };

            // 측면 4면. 각 면은 수평 에지 (b0 → b1) × 세로 링으로 구성 — 큐브와 동일한
            // CW 외향 와인딩(LH): 세그먼트 쿼드 BL(b0,y0) BR(b1,y0) TR(b1,y1) TL(b0,y1), 삼각형 (0,2,1)(0,3,2).
            struct SideFace
            {
                float normal[3];
                float b0[2]; // (x, z) — 면 하단 왼쪽 끝
                float b1[2]; // (x, z) — 면 하단 오른쪽 끝
            };
            const SideFace kSides[4] =
            {
                { { 0, 0,  1 }, { -HALF,  HALF }, {  HALF,  HALF } }, // +Z
                { { 0, 0, -1 }, {  HALF, -HALF }, { -HALF, -HALF } }, // -Z
                { {  1, 0, 0 }, {  HALF,  HALF }, {  HALF, -HALF } }, // +X
                { { -1, 0, 0 }, { -HALF, -HALF }, { -HALF,  HALF } }, // -X
            };

            for (const SideFace& side : kSides)
            {
                for (int seg = 0; seg < SEGMENTS; ++seg)
                {
                    const float y0 = seg * SEG_DY;
                    const float y1 = y0 + SEG_DY;
                    const float v0 = static_cast<float>(seg) / SEGMENTS;
                    const float v1 = static_cast<float>(seg + 1) / SEGMENTS;
                    const uint32_t base = static_cast<uint32_t>(_vertices.size());

                    pushVertex(side.b0[0], y0, side.b0[1], side.normal[0], side.normal[1], side.normal[2], 0.0f, v0);
                    pushVertex(side.b1[0], y0, side.b1[1], side.normal[0], side.normal[1], side.normal[2], 1.0f, v0);
                    pushVertex(side.b1[0], y1, side.b1[1], side.normal[0], side.normal[1], side.normal[2], 1.0f, v1);
                    pushVertex(side.b0[0], y1, side.b0[1], side.normal[0], side.normal[1], side.normal[2], 0.0f, v1);

                    _indices.push_back(base + 0);
                    _indices.push_back(base + 2);
                    _indices.push_back(base + 1);
                    _indices.push_back(base + 0);
                    _indices.push_back(base + 3);
                    _indices.push_back(base + 2);
                }
            }

            // 상/하 캡 (큐브 ±Y 면과 동일 와인딩).
            const uint32_t topBase = static_cast<uint32_t>(_vertices.size());
            pushVertex(-HALF, HEIGHT,  HALF, 0, 1, 0, 0, 0);
            pushVertex( HALF, HEIGHT,  HALF, 0, 1, 0, 1, 0);
            pushVertex( HALF, HEIGHT, -HALF, 0, 1, 0, 1, 1);
            pushVertex(-HALF, HEIGHT, -HALF, 0, 1, 0, 0, 1);
            const uint32_t bottomBase = static_cast<uint32_t>(_vertices.size());
            pushVertex(-HALF, 0.0f, -HALF, 0, -1, 0, 0, 0);
            pushVertex( HALF, 0.0f, -HALF, 0, -1, 0, 1, 0);
            pushVertex( HALF, 0.0f,  HALF, 0, -1, 0, 1, 1);
            pushVertex(-HALF, 0.0f,  HALF, 0, -1, 0, 0, 1);
            for (uint32_t capBase : { topBase, bottomBase })
            {
                _indices.push_back(capBase + 0);
                _indices.push_back(capBase + 2);
                _indices.push_back(capBase + 1);
                _indices.push_back(capBase + 0);
                _indices.push_back(capBase + 3);
                _indices.push_back(capBase + 2);
            }
        }
    }

    std::shared_ptr<StaticMesh> MiniLoader::LoadStaticMesh(const std::wstring& _path)
    {
        std::ifstream file(_path, std::ios::binary);
        if (!file.is_open())
        {
            MG_LOG_ERROR("MiniLoader: failed to open file for read");
            return nullptr;
        }

        MiniHeader header = {};
        file.read(reinterpret_cast<char*>(&header), sizeof(header));
        if (!file || header.magic != MINI_MAGIC)
        {
            MG_LOG_ERROR("MiniLoader: invalid MAGIC (not a .mini file)");
            return nullptr;
        }
        if (header.version != MINI_VERSION)
        {
            MG_LOG_ERROR("MiniLoader: unsupported version {}", header.version);
            return nullptr;
        }
        if (header.assetType != static_cast<uint32_t>(MiniAssetType::StaticMesh))
        {
            MG_LOG_ERROR("MiniLoader: assetType {} is not StaticMesh", header.assetType);
            return nullptr;
        }

        MiniStaticMeshHeader meshHeader = {};
        file.read(reinterpret_cast<char*>(&meshHeader), sizeof(meshHeader));
        if (!file || meshHeader.vertexCount == 0 || meshHeader.indexCount == 0)
        {
            MG_LOG_ERROR("MiniLoader: empty or corrupt StaticMesh header");
            return nullptr;
        }

        std::vector<MiniStaticVertex> vertices(meshHeader.vertexCount);
        std::vector<uint32_t>         indices(meshHeader.indexCount);
        file.read(reinterpret_cast<char*>(vertices.data()),
                  static_cast<std::streamsize>(vertices.size() * sizeof(MiniStaticVertex)));
        file.read(reinterpret_cast<char*>(indices.data()),
                  static_cast<std::streamsize>(indices.size() * sizeof(uint32_t)));
        if (!file)
        {
            MG_LOG_ERROR("MiniLoader: truncated StaticMesh body");
            return nullptr;
        }

        auto mesh = std::make_shared<StaticMesh>();
        mesh->SetData(std::move(vertices), std::move(indices));
        MG_LOG_INFO("MiniLoader: loaded StaticMesh ({} verts, {} indices)",
                    meshHeader.vertexCount, meshHeader.indexCount);
        return mesh;
    }

    bool MiniLoader::WriteStaticMesh(const std::wstring& _path,
                                     const std::vector<MiniStaticVertex>& _vertices,
                                     const std::vector<uint32_t>& _indices)
    {
        if (_vertices.empty() || _indices.empty())
        {
            MG_LOG_ERROR("MiniLoader: refusing to write empty StaticMesh");
            return false;
        }

        // 출력 부모 디렉터리가 없으면 생성(예: Baked\).
        std::error_code ec;
        const std::filesystem::path parent = std::filesystem::path(_path).parent_path();
        if (!parent.empty())
            std::filesystem::create_directories(parent, ec);

        std::ofstream file(_path, std::ios::binary | std::ios::trunc);
        if (!file.is_open())
        {
            MG_LOG_ERROR("MiniLoader: failed to open file for write");
            return false;
        }

        MiniHeader header = {};
        header.magic     = MINI_MAGIC;
        header.version   = MINI_VERSION;
        header.assetType = static_cast<uint32_t>(MiniAssetType::StaticMesh);
        header.reserved  = 0;

        MiniStaticMeshHeader meshHeader = {};
        meshHeader.vertexCount = static_cast<uint32_t>(_vertices.size());
        meshHeader.indexCount  = static_cast<uint32_t>(_indices.size());

        file.write(reinterpret_cast<const char*>(&header), sizeof(header));
        file.write(reinterpret_cast<const char*>(&meshHeader), sizeof(meshHeader));
        file.write(reinterpret_cast<const char*>(_vertices.data()),
                   static_cast<std::streamsize>(_vertices.size() * sizeof(MiniStaticVertex)));
        file.write(reinterpret_cast<const char*>(_indices.data()),
                   static_cast<std::streamsize>(_indices.size() * sizeof(uint32_t)));
        if (!file)
        {
            MG_LOG_ERROR("MiniLoader: write failed");
            return false;
        }

        MG_LOG_INFO("MiniLoader: wrote StaticMesh .mini ({} verts, {} indices)",
                    meshHeader.vertexCount, meshHeader.indexCount);
        return true;
    }

    bool MiniLoader::WriteCubeMini(const std::wstring& _path)
    {
        std::vector<MiniStaticVertex> vertices;
        std::vector<uint32_t>         indices;
        BuildCube(vertices, indices);
        return WriteStaticMesh(_path, vertices, indices);
    }

    std::shared_ptr<SkinnedMesh> MiniLoader::LoadSkinnedMesh(const std::wstring& _path)
    {
        std::ifstream file(_path, std::ios::binary);
        if (!file.is_open())
        {
            MG_LOG_ERROR("MiniLoader: failed to open file for read");
            return nullptr;
        }

        MiniHeader header = {};
        file.read(reinterpret_cast<char*>(&header), sizeof(header));
        if (!file || header.magic != MINI_MAGIC)
        {
            MG_LOG_ERROR("MiniLoader: invalid MAGIC (not a .mini file)");
            return nullptr;
        }
        if (header.version != MINI_VERSION)
        {
            MG_LOG_ERROR("MiniLoader: unsupported version {}", header.version);
            return nullptr;
        }
        if (header.assetType != static_cast<uint32_t>(MiniAssetType::SkinnedMesh))
        {
            MG_LOG_ERROR("MiniLoader: assetType {} is not SkinnedMesh", header.assetType);
            return nullptr;
        }

        MiniSkinnedMeshHeader meshHeader = {};
        file.read(reinterpret_cast<char*>(&meshHeader), sizeof(meshHeader));
        if (!file || meshHeader.vertexCount == 0 || meshHeader.indexCount == 0 || meshHeader.boneCount == 0)
        {
            MG_LOG_ERROR("MiniLoader: empty or corrupt SkinnedMesh header");
            return nullptr;
        }

        // 정점/인덱스.
        std::vector<MiniSkinnedVertex> vertices(meshHeader.vertexCount);
        std::vector<uint32_t>          indices(meshHeader.indexCount);
        file.read(reinterpret_cast<char*>(vertices.data()),
                  static_cast<std::streamsize>(vertices.size() * sizeof(MiniSkinnedVertex)));
        file.read(reinterpret_cast<char*>(indices.data()),
                  static_cast<std::streamsize>(indices.size() * sizeof(uint32_t)));
        if (!file)
        {
            MG_LOG_ERROR("MiniLoader: truncated SkinnedMesh body");
            return nullptr;
        }

        // 스켈레톤. 부모 인덱스는 항상 자기보다 앞이어야 함(ComputeBoneMatrices 전제).
        Skeleton skeleton;
        skeleton.bones.resize(meshHeader.boneCount);
        for (uint32_t i = 0; i < meshHeader.boneCount; ++i)
        {
            MiniBone raw = {};
            file.read(reinterpret_cast<char*>(&raw), sizeof(raw));
            if (!file || raw.parentIndex >= static_cast<int32_t>(i))
            {
                MG_LOG_ERROR("MiniLoader: truncated or ill-ordered bone {}", i);
                return nullptr;
            }
            Bone& bone = skeleton.bones[i];
            bone.parentIndex     = raw.parentIndex;
            bone.localBindPose   = FloatsToMatrix(raw.localBindPose);
            bone.inverseBindPose = FloatsToMatrix(raw.inverseBindPose);
            bone.name            = ReadName(raw.name);
        }

        // 클립.
        std::vector<AnimClip> clips(meshHeader.clipCount);
        for (uint32_t c = 0; c < meshHeader.clipCount; ++c)
        {
            MiniAnimClipHeader clipHeader = {};
            file.read(reinterpret_cast<char*>(&clipHeader), sizeof(clipHeader));
            if (!file)
            {
                MG_LOG_ERROR("MiniLoader: truncated clip header {}", c);
                return nullptr;
            }

            AnimClip& clip = clips[c];
            clip.name           = ReadName(clipHeader.name);
            clip.duration       = clipHeader.duration;
            clip.ticksPerSecond = clipHeader.ticksPerSecond;
            clip.channels.resize(clipHeader.channelCount);

            for (uint32_t ch = 0; ch < clipHeader.channelCount; ++ch)
            {
                MiniAnimChannelHeader channelHeader = {};
                file.read(reinterpret_cast<char*>(&channelHeader), sizeof(channelHeader));
                if (!file || channelHeader.boneIndex >= meshHeader.boneCount)
                {
                    MG_LOG_ERROR("MiniLoader: truncated or corrupt channel header");
                    return nullptr;
                }

                AnimChannel& channel = clip.channels[ch];
                channel.boneIndex = static_cast<int>(channelHeader.boneIndex);
                channel.pos.resize(channelHeader.posKeyCount);
                channel.rot.resize(channelHeader.rotKeyCount);
                channel.scale.resize(channelHeader.scaleKeyCount);

                for (VecKey& key : channel.pos)
                {
                    MiniVecKey raw = {};
                    file.read(reinterpret_cast<char*>(&raw), sizeof(raw));
                    key.time  = raw.time;
                    key.value = Vector3(raw.v[0], raw.v[1], raw.v[2]);
                }
                for (QuatKey& key : channel.rot)
                {
                    MiniQuatKey raw = {};
                    file.read(reinterpret_cast<char*>(&raw), sizeof(raw));
                    key.time  = raw.time;
                    key.value = Quaternion(raw.v[0], raw.v[1], raw.v[2], raw.v[3]);
                }
                for (VecKey& key : channel.scale)
                {
                    MiniVecKey raw = {};
                    file.read(reinterpret_cast<char*>(&raw), sizeof(raw));
                    key.time  = raw.time;
                    key.value = Vector3(raw.v[0], raw.v[1], raw.v[2]);
                }
                if (!file)
                {
                    MG_LOG_ERROR("MiniLoader: truncated key data");
                    return nullptr;
                }
            }
        }

        auto mesh = std::make_shared<SkinnedMesh>();
        mesh->SetData(std::move(vertices), std::move(indices));
        mesh->SetSkeleton(std::move(skeleton));
        mesh->SetClips(std::move(clips));
        MG_LOG_INFO("MiniLoader: loaded SkinnedMesh ({} verts, {} indices, {} bones, {} clips)",
                    meshHeader.vertexCount, meshHeader.indexCount, meshHeader.boneCount, meshHeader.clipCount);
        return mesh;
    }

    bool MiniLoader::WriteSkinnedMesh(const std::wstring& _path,
                                      const std::vector<MiniSkinnedVertex>& _vertices,
                                      const std::vector<uint32_t>& _indices,
                                      const Skeleton& _skeleton,
                                      const std::vector<AnimClip>& _clips)
    {
        if (_vertices.empty() || _indices.empty() || _skeleton.bones.empty())
        {
            MG_LOG_ERROR("MiniLoader: refusing to write empty SkinnedMesh");
            return false;
        }

        // 출력 부모 디렉터리가 없으면 생성 (WriteStaticMesh 와 동일).
        std::error_code ec;
        const std::filesystem::path parent = std::filesystem::path(_path).parent_path();
        if (!parent.empty())
            std::filesystem::create_directories(parent, ec);

        std::ofstream file(_path, std::ios::binary | std::ios::trunc);
        if (!file.is_open())
        {
            MG_LOG_ERROR("MiniLoader: failed to open file for write");
            return false;
        }

        MiniHeader header = {};
        header.magic     = MINI_MAGIC;
        header.version   = MINI_VERSION;
        header.assetType = static_cast<uint32_t>(MiniAssetType::SkinnedMesh);
        header.reserved  = 0;

        MiniSkinnedMeshHeader meshHeader = {};
        meshHeader.vertexCount = static_cast<uint32_t>(_vertices.size());
        meshHeader.indexCount  = static_cast<uint32_t>(_indices.size());
        meshHeader.boneCount   = static_cast<uint32_t>(_skeleton.bones.size());
        meshHeader.clipCount   = static_cast<uint32_t>(_clips.size());

        file.write(reinterpret_cast<const char*>(&header), sizeof(header));
        file.write(reinterpret_cast<const char*>(&meshHeader), sizeof(meshHeader));
        file.write(reinterpret_cast<const char*>(_vertices.data()),
                   static_cast<std::streamsize>(_vertices.size() * sizeof(MiniSkinnedVertex)));
        file.write(reinterpret_cast<const char*>(_indices.data()),
                   static_cast<std::streamsize>(_indices.size() * sizeof(uint32_t)));

        for (const Bone& bone : _skeleton.bones)
        {
            MiniBone raw = {};
            raw.parentIndex = bone.parentIndex;
            MatrixToFloats(bone.localBindPose, raw.localBindPose);
            MatrixToFloats(bone.inverseBindPose, raw.inverseBindPose);
            CopyName(raw.name, bone.name);
            file.write(reinterpret_cast<const char*>(&raw), sizeof(raw));
        }

        for (const AnimClip& clip : _clips)
        {
            MiniAnimClipHeader clipHeader = {};
            CopyName(clipHeader.name, clip.name);
            clipHeader.duration       = clip.duration;
            clipHeader.ticksPerSecond = clip.ticksPerSecond;
            clipHeader.channelCount   = static_cast<uint32_t>(clip.channels.size());
            file.write(reinterpret_cast<const char*>(&clipHeader), sizeof(clipHeader));

            for (const AnimChannel& channel : clip.channels)
            {
                MiniAnimChannelHeader channelHeader = {};
                channelHeader.boneIndex     = static_cast<uint32_t>(channel.boneIndex);
                channelHeader.posKeyCount   = static_cast<uint32_t>(channel.pos.size());
                channelHeader.rotKeyCount   = static_cast<uint32_t>(channel.rot.size());
                channelHeader.scaleKeyCount = static_cast<uint32_t>(channel.scale.size());
                file.write(reinterpret_cast<const char*>(&channelHeader), sizeof(channelHeader));

                for (const VecKey& key : channel.pos)
                {
                    const MiniVecKey raw = { key.time, { key.value.x, key.value.y, key.value.z } };
                    file.write(reinterpret_cast<const char*>(&raw), sizeof(raw));
                }
                for (const QuatKey& key : channel.rot)
                {
                    const MiniQuatKey raw = { key.time, { key.value.x, key.value.y, key.value.z, key.value.w } };
                    file.write(reinterpret_cast<const char*>(&raw), sizeof(raw));
                }
                for (const VecKey& key : channel.scale)
                {
                    const MiniVecKey raw = { key.time, { key.value.x, key.value.y, key.value.z } };
                    file.write(reinterpret_cast<const char*>(&raw), sizeof(raw));
                }
            }
        }

        if (!file)
        {
            MG_LOG_ERROR("MiniLoader: write failed");
            return false;
        }

        MG_LOG_INFO("MiniLoader: wrote SkinnedMesh .mini ({} verts, {} indices, {} bones, {} clips)",
                    meshHeader.vertexCount, meshHeader.indexCount, meshHeader.boneCount, meshHeader.clipCount);
        return true;
    }

    bool MiniLoader::WriteSkinnedTest(const std::wstring& _path)
    {
        std::vector<MiniSkinnedVertex> vertices;
        std::vector<uint32_t>          indices;
        BuildSkinnedTestBox(vertices, indices);

        // 스켈레톤: bone0(root, y=0) + bone1(자식, y=2 관절). 바인드는 순수 이동이므로
        // 글로벌 바인드 역행렬 = 역방향 이동.
        Skeleton skeleton;
        skeleton.bones.resize(2);
        skeleton.bones[0].parentIndex     = -1;
        skeleton.bones[0].name            = "root";
        skeleton.bones[1].parentIndex     = 0;
        skeleton.bones[1].localBindPose   = Matrix::CreateTranslation(0.0f, 2.0f, 0.0f);
        skeleton.bones[1].inverseBindPose = Matrix::CreateTranslation(0.0f, -2.0f, 0.0f);
        skeleton.bones[1].name            = "upper";

        // 클립 = bone1 왕복 회전(0° → +40° → 0° → −40° → 0°, 2초 루프). 축만 다름:
        // "wave"(Z축) / "twist"(Y축) — 크로스페이드가 시각적으로 구분되도록 2개 수록.
        const struct { const char* name; Vector3 axis; } kClipDefs[] = {
            { "wave",  Vector3(0.0f, 0.0f, 1.0f) },
            { "twist", Vector3(0.0f, 1.0f, 0.0f) },
        };

        std::vector<AnimClip> clips;
        for (const auto& def : kClipDefs)
        {
            AnimClip clip;
            clip.name           = def.name;
            clip.duration       = 2.0f;
            clip.ticksPerSecond = 1.0f;

            AnimChannel channel;
            channel.boneIndex = 1;
            const float kTimes[5]  = { 0.0f, 0.5f, 1.0f, 1.5f, 2.0f };
            const float kAngles[5] = { 0.0f, ToRadians(40.0f), 0.0f, ToRadians(-40.0f), 0.0f };
            for (int i = 0; i < 5; ++i)
            {
                QuatKey key;
                key.time  = kTimes[i];
                key.value = Quaternion::CreateFromAxisAngle(def.axis, kAngles[i]);
                channel.rot.push_back(key);
            }
            // pos/scale 트랙은 비움 → 바인드 포즈 성분(이동 y=2, 스케일 1) 유지.
            clip.channels.push_back(std::move(channel));
            clips.push_back(std::move(clip));
        }

        return WriteSkinnedMesh(_path, vertices, indices, skeleton, clips);
    }
}
