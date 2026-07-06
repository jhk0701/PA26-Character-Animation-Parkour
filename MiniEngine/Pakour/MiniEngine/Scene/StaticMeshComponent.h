#pragma once
#include <memory>
#include "Scene/SceneComponent.h"
#include "Asset/StaticMesh.h"

namespace MiniEngine
{
    // 정적 메시를 렌더링 대상으로 보유하는 SceneComponent. (CLAUDE.md §6)
    // 메시 핸들만 보유(데이터 컴포넌트) — 실제 드로우는 GameCore/렌더러가 순회하며 수행.
    // 트랜스폼/월드행렬은 SceneComponent 에서 상속(GetWorldMatrix).
    class StaticMeshComponent : public SceneComponent
    {
    public:
        void SetMesh(const std::shared_ptr<StaticMesh>& _mesh) { m_mesh = _mesh; }
        const std::shared_ptr<StaticMesh>& GetMesh() const { return m_mesh; }

    private:
        std::shared_ptr<StaticMesh> m_mesh; // 사용처 소유(shared). (§12)
    };
}
