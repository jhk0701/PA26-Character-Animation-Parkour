#include "pch.h"
#include "Scene/Actor.h"

namespace MiniEngine
{
    void Actor::Tick(float _dt)
    {
        for (std::shared_ptr<Component>& comp : m_components)
            comp->Tick(_dt);
    }

    void Actor::Render(Graphics::RenderContext& _context)
    {
        for (std::shared_ptr<Component>& comp : m_components)
            comp->Render(_context);
    }
    
    void Actor::OnSpawned(std::weak_ptr<Scene> _scene)
    {
        m_scene = _scene;
    }
}
