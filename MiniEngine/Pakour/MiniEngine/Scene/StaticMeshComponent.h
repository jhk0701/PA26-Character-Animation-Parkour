#pragma once
#include <memory>
#include "Scene/SceneComponent.h"
#include "Asset/StaticMesh.h"

namespace MiniEngine
{
    class StaticMeshComponent : public SceneComponent
    {
    public:
        void Render(Graphics::RenderContext& _context) override;
        
        void SetMesh(const std::shared_ptr<StaticMesh>& _mesh) { m_mesh = _mesh; }
        const std::shared_ptr<StaticMesh>& GetMesh() const { return m_mesh; }

    private:
        std::shared_ptr<StaticMesh> m_mesh;
    };
}
