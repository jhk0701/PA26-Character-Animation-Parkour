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

    void Actor::SetParent(std::weak_ptr<Actor> _newParent)
    {
        // 이전 부모가 있는 경우?
        if (m_parent.expired() == false)
            m_parent.lock()->RemoveChild(shared_from_this()); // 이전 부모로부터 자식 등록 제거

        m_parent = _newParent; // 새 부모 등록
    }

    void Actor::AddChild(std::weak_ptr<Actor> _child)
    {
        if (_child.expired())
            return;

        _child.lock()->SetParent(shared_from_this());
        m_children.push_back(_child);
    }

    void Actor::RemoveChild(std::weak_ptr<Actor> _child)
    {
        for (int i = 0; i < m_children.size(); ++i)
        {
            std::weak_ptr<Actor>& child = m_children[i];
            if (child.expired())
                continue;

            if (_child.lock() != child.lock())
                continue;

            // 사이즈는 줄이지 않음
            // 필요하면 줄이는게 나을 듯
            m_children[i].reset(); 
        }
    }
}
