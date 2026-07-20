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

        void SetColor(const Vector3& _newColor) { m_albedo = _newColor; }

    private:
        std::shared_ptr<StaticMesh> m_mesh;
        Vector3 m_albedo{ 0.5f, 0.5f, 0.5f };
    };
}
