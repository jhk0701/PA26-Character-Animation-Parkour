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
        // SkinnedMesh 직렬화

        // SimpleMath Matrix(row-major 저장) ↔ float[16] 크기 동일
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
            
            if (len < _src.size())
                MG_LOG_WARN("MiniLoader: 이름이 {}자에서 절단됨 — \"{}\" (앞 {}자가 같은 이름과 충돌 가능)",
                    MINI_NAME_LENGTH - 1, _src, MINI_NAME_LENGTH - 1);
        }

        // 널 종단 보장 읽기
        std::string ReadName(const char (&_src)[MINI_NAME_LENGTH])
        {
            const size_t len = ::strnlen(_src, MINI_NAME_LENGTH);
            return std::string(_src, len);
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
        header.magic = MINI_MAGIC;
        header.version = MINI_VERSION;
        header.assetType = static_cast<uint32_t>(MiniAssetType::StaticMesh);
        header.bakeFlags = 0; // StaticMesh 는 스켈레톤/루트모션이 없어 축 정규화 개념이 없다.

        MiniStaticMeshHeader meshHeader = {};
        meshHeader.vertexCount = static_cast<uint32_t>(_vertices.size());
        meshHeader.indexCount = static_cast<uint32_t>(_indices.size());

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
            bone.parentIndex = raw.parentIndex;
            bone.localBindPose = FloatsToMatrix(raw.localBindPose);
            bone.inverseBindPose = FloatsToMatrix(raw.inverseBindPose);
            bone.name = ReadName(raw.name);
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
            clip.name = ReadName(clipHeader.name);
            clip.duration = clipHeader.duration;
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
                    key.time = raw.time;
                    key.value = Vector3(raw.v[0], raw.v[1], raw.v[2]);
                }
                for (QuatKey& key : channel.rot)
                {
                    MiniQuatKey raw = {};
                    file.read(reinterpret_cast<char*>(&raw), sizeof(raw));
                    key.time = raw.time;
                    key.value = Quaternion(raw.v[0], raw.v[1], raw.v[2], raw.v[3]);
                }
                for (VecKey& key : channel.scale)
                {
                    MiniVecKey raw = {};
                    file.read(reinterpret_cast<char*>(&raw), sizeof(raw));
                    key.time = raw.time;
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
        mesh->SetBakeFlags(header.bakeFlags); // 편집·재저장 라운드트립용(구 파일 = 0).
        MG_LOG_INFO("MiniLoader: loaded SkinnedMesh ({} verts, {} indices, {} bones, {} clips)",
            meshHeader.vertexCount, meshHeader.indexCount, meshHeader.boneCount, meshHeader.clipCount);
        if ((header.bakeFlags & MINI_BAKE_AXIS_NORMALIZED) == 0)
            MG_LOG_WARN("MiniLoader: SkinnedMesh was baked before axis normalization — root motion may be "
                "inaccurate (rebake recommended; see docs/UEFN_Bone.md §5.8)");
        return mesh;
    }

    bool MiniLoader::WriteSkinnedMesh(const std::wstring& _path,
                                      const std::vector<MiniSkinnedVertex>& _vertices,
                                      const std::vector<uint32_t>& _indices,
                                      const Skeleton& _skeleton,
                                      const std::vector<AnimClip>& _clips,
                                      uint32_t _bakeFlags)
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
        header.magic = MINI_MAGIC;
        header.version = MINI_VERSION;
        header.assetType = static_cast<uint32_t>(MiniAssetType::SkinnedMesh);
        header.bakeFlags = _bakeFlags;

        MiniSkinnedMeshHeader meshHeader = {};
        meshHeader.vertexCount = static_cast<uint32_t>(_vertices.size());
        meshHeader.indexCount = static_cast<uint32_t>(_indices.size());
        meshHeader.boneCount = static_cast<uint32_t>(_skeleton.bones.size());
        meshHeader.clipCount = static_cast<uint32_t>(_clips.size());

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
            clipHeader.duration = clip.duration;
            clipHeader.ticksPerSecond = clip.ticksPerSecond;
            clipHeader.channelCount = static_cast<uint32_t>(clip.channels.size());
            file.write(reinterpret_cast<const char*>(&clipHeader), sizeof(clipHeader));

            for (const AnimChannel& channel : clip.channels)
            {
                MiniAnimChannelHeader channelHeader = {};
                channelHeader.boneIndex = static_cast<uint32_t>(channel.boneIndex);
                channelHeader.posKeyCount = static_cast<uint32_t>(channel.pos.size());
                channelHeader.rotKeyCount = static_cast<uint32_t>(channel.rot.size());
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

    bool MiniLoader::PeekHeader(const std::wstring& _path, MiniHeader& _outHeader)
    {
        std::ifstream file(_path, std::ios::binary);
        if (!file.is_open())
            return false;
        file.read(reinterpret_cast<char*>(&_outHeader), sizeof(_outHeader));
        return static_cast<bool>(file)
            && _outHeader.magic == MINI_MAGIC
            && _outHeader.version == MINI_VERSION;
    }
}
