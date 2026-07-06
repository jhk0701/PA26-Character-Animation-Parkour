#include "pch.h"
#include "Asset/AssetManager.h"
#include "Asset/MiniLoader.h"
#include "Core/Log.h"

namespace MiniEngine
{
    std::shared_ptr<StaticMesh> AssetManager::LoadStaticMesh(const std::wstring& _path, ID3D11Device* _device)
    {
        // 캐시 히트: 아직 살아있는 핸들이 있으면 그대로 공유.
        auto it = m_staticMeshCache.find(_path);
        if (it != m_staticMeshCache.end())
        {
            if (auto cached = it->second.lock())
                return cached;
        }

        // 로드(CPU 데이터) → GPU 리소스 생성.
        auto mesh = MiniLoader::LoadStaticMesh(_path);
        if (!mesh)
            return nullptr;

        if (!mesh->CreateGpuResources(_device))
        {
            MG_LOG_ERROR("AssetManager: CreateGpuResources failed");
            return nullptr;
        }

        m_staticMeshCache[_path] = mesh; // weak 캐시 등록/갱신
        return mesh;
    }

    std::shared_ptr<SkinnedMesh> AssetManager::LoadSkinnedMesh(const std::wstring& _path, ID3D11Device* _device)
    {
        // 캐시 히트: 아직 살아있는 핸들이 있으면 그대로 공유.
        auto it = m_skinnedMeshCache.find(_path);
        if (it != m_skinnedMeshCache.end())
        {
            if (auto cached = it->second.lock())
                return cached;
        }

        // 로드(CPU 데이터) → GPU 리소스 생성.
        auto mesh = MiniLoader::LoadSkinnedMesh(_path);
        if (!mesh)
            return nullptr;

        if (!mesh->CreateGpuResources(_device))
        {
            MG_LOG_ERROR("AssetManager: CreateGpuResources failed (SkinnedMesh)");
            return nullptr;
        }

        m_skinnedMeshCache[_path] = mesh; // weak 캐시 등록/갱신
        return mesh;
    }
}
