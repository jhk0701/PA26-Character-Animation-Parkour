#pragma once
#include <vector>
#include <memory>
#include "Scene/Component.h"
#include "Scene/Transform.h"

namespace MiniEngine
{
    class SceneComponent : public Component, public std::enable_shared_from_this<SceneComponent>
    {
    public:
        virtual ~SceneComponent() {};

        Transform localTransform;

        void AttachTo(const std::shared_ptr<SceneComponent>& _parent);
        Matrix GetWorldMatrix() const;

        std::shared_ptr<SceneComponent> GetParent() const { return m_parent.lock(); }
        const std::vector<std::weak_ptr<SceneComponent>>& GetChildren() const { return m_children; }

    private:
        std::weak_ptr<SceneComponent> m_parent;
        std::vector<std::weak_ptr<SceneComponent>> m_children;
    };
}
