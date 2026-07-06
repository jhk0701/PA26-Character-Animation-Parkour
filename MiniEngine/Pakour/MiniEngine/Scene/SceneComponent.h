#pragma once
#include <vector>
#include <memory>
#include "Scene/Component.h"
#include "Scene/Transform.h"

namespace MiniEngine
{
    // 트랜스폼을 갖는 컴포넌트. attach로 계층(부모/자식)을 형성한다. (CLAUDE.md §6)
    // 소유는 Actor(단독). 씬 그래프 parent/children 링크는 비소유(weak_ptr)로 표현한다. (§12)
    class SceneComponent : public Component, public std::enable_shared_from_this<SceneComponent>
    {
    public:
        Transform localTransform;

        // 이 컴포넌트를 다른 SceneComponent의 자식으로 붙인다.
        void AttachTo(const std::shared_ptr<SceneComponent>& _parent);

        // 월드 행렬. row-vector 관례이므로 local 을 먼저 적용한다:
        //   world = local * parent->World
        // (CLAUDE.md §6 주석의 "parent->World * local"은 column-vector 표기 — 코드는 순서가 반대.)
        Matrix GetWorldMatrix() const;

        // 부모(있으면). 비소유 링크이므로 lock한 shared_ptr을 반환(만료 시 nullptr).
        std::shared_ptr<SceneComponent> GetParent() const { return m_parent.lock(); }
        const std::vector<std::weak_ptr<SceneComponent>>& GetChildren() const { return m_children; }

    private:
        std::weak_ptr<SceneComponent> m_parent;
        std::vector<std::weak_ptr<SceneComponent>> m_children;
    };
}
