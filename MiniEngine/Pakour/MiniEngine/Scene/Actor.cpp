#include "pch.h"
#include "Scene/Actor.h"
#include "Core/Log.h"

namespace MiniEngine
{
    void Actor::Tick(float _dt)
    {
        for (std::shared_ptr<Component>& comp : m_components)
            comp->Tick(_dt);
    }

    void Actor::FixedTick(float _dt)
    {
        for (std::shared_ptr<Component>& comp : m_components)
            comp->FixedTick(_dt);
    }

    void Actor::Render(Graphics::RenderContext& _context)
    {
        for (std::shared_ptr<Component>& comp : m_components)
            comp->Render(_context);
    }

    void Actor::SetRoot(std::shared_ptr<SceneComponent>& _newRoot)
    {
        bool ownned = false;
        for (std::shared_ptr<Component> pComp : m_components)
        {
            if (pComp == _newRoot)
            {
                ownned = true;
                break;
            }
        }

        if (!ownned)
        {
            MG_LOG_ERROR("[Actor::SetRoot] _newRoot is not included in this actor.");
            return;
        }

        m_root = _newRoot;
    }
    
    void Actor::OnSpawned(std::weak_ptr<Scene> _scene)
    {
        m_scene = _scene;
    }

}
