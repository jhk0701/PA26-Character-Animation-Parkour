#include "pch.h"
#include "Scene/Actor.h"
#include "Core/Log.h"

namespace MiniEngine
{
    void Actor::BeginPlay()
    {
        OnBeforeSortComponent();

        // 컴포넌트 정렬
        std::sort(m_components.begin(), m_components.end(),
            [](const std::shared_ptr<Component> _a, const std::shared_ptr<Component> _b) 
            { return _a->GetSortOrder() > _b->GetSortOrder(); }
        );
    }

    void Actor::FixedTick(float _dt)
    {
        if (!m_bUseFixedTick)
            return;

        for (std::shared_ptr<Component>& comp : m_components)
            comp->FixedTick(_dt);
    }

    void Actor::Tick(float _dt)
    {
        if (!m_bUseTick)
            return;

        for (std::shared_ptr<Component>& comp : m_components)
            comp->Tick(_dt);
    }

    void Actor::LateTick(float _dt)
    {
        if (!m_bUseLateTick)
            return;

        for (std::shared_ptr<Component>& comp : m_components)
            comp->LateTick(_dt);
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

    void Actor::SetTickConfig(bool _bUseFixedTick, bool _bUseTick, bool _bUseLateTick)
    {
        m_bUseFixedTick = _bUseFixedTick;
        m_bUseTick = _bUseTick;
        m_bUseLateTick = _bUseLateTick;
    };

}
