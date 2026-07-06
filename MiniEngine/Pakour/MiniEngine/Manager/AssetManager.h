#pragma once
#include <string>
#include <memory>
#include <unordered_map>
#include <d3d11.h>

#include "Asset/MiniLoader.h"
#include "Asset/StaticMesh.h"
#include "Asset/SkinnedMesh.h"

namespace MiniEngine
{
    // 경로 기반 애셋 핸들 발급 + 참조 카운팅(중복 로드 방지). (CLAUDE.md §8/§12)
    // 캐시는 weak_ptr 로 보관 → 사용처가 모두 해제되면 자동 만료(비소유).
    // 소유는 반환된 shared_ptr(사용처, 예: StaticMeshComponent)에 있다.
    class AssetManager
    {
        SINGLETON(AssetManager)

    public:
        void Init(ID3D11Device* _device);
        bool IsInitialized() const { return m_pDevice != nullptr; }

        bool IsValidPath(const std::wstring& _path) const;

        // .mini StaticMesh 로드(+ GPU 리소스 생성). 이미 살아있으면 캐시된 핸들 반환.
        // 실패 시 nullptr.
        std::shared_ptr<StaticMesh> LoadStaticMesh(const std::wstring& _path);

        // .mini SkinnedMesh(통합 컨테이너) 로드(+ GPU 리소스 생성). 캐시 규칙 동일.
        std::shared_ptr<SkinnedMesh> LoadSkinnedMesh(const std::wstring& _path);

    private:
        ID3D11Device* m_pDevice{ nullptr };

        std::unordered_map<std::wstring, std::weak_ptr<StaticMesh>>  m_staticMeshCache;
        std::unordered_map<std::wstring, std::weak_ptr<SkinnedMesh>> m_skinnedMeshCache;
    };
}
