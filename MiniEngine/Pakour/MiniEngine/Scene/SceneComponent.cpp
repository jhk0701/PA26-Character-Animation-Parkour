#include "pch.h"
#include "Scene/SceneComponent.h"

namespace MiniEngine
{
    void SceneComponent::AttachTo(const std::shared_ptr<SceneComponent>& _parent)
    {
        // 기존 부모에서 자기 자신(및 만료된 링크)을 제거.
        if (auto oldParent = m_parent.lock())
        {
            auto& siblings = oldParent->m_children;
            siblings.erase(
                std::remove_if(siblings.begin(), siblings.end(),
                    [this](const std::weak_ptr<SceneComponent>& _w)
                    {
                        auto s = _w.lock();
                        return !s || s.get() == this;
                    }),
                siblings.end());
        }

        m_parent = _parent;
        if (_parent)
            _parent->m_children.push_back(weak_from_this());
    }

    Matrix SceneComponent::GetWorldMatrix() const
    {
        const Matrix local = localTransform.GetMatrix();
        if (auto parent = m_parent.lock())
            return local * parent->GetWorldMatrix(); // local 먼저 (row-vector)
        return local;
    }
}
