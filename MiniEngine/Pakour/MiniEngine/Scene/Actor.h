#pragma once
#include <vector>
#include <memory>
#include <string>
#include <type_traits>
#include "Scene/Component.h"
#include "Scene/SceneComponent.h"

namespace MiniEngine
{
    class Actor : public std::enable_shared_from_this<Actor>
    {
    public:
        virtual ~Actor() = default;

        virtual void BeginPlay() {}
        virtual void Tick(float _dt); // 소유 컴포넌트 Tick 전파
        virtual void Render();
        virtual void EndPlay() {}

        // 컴포넌트 추가. owner(weak) 설정 + (첫 SceneComponent면) root 자동 지정 + OnAttach 호출.
        template<typename T, typename... Args>
        std::shared_ptr<T> AddComponent(Args&&... _args)
        {
            static_assert(std::is_base_of<Component, T>::value, "T must derive from Component");
            auto comp = std::make_shared<T>(std::forward<Args>(_args)...);
            comp->owner = weak_from_this();

            if (m_root.expired())
            {
                if (auto sc = std::dynamic_pointer_cast<SceneComponent>(comp))
                    m_root = sc;
            }

            m_components.push_back(comp);
            comp->OnAttach();
            return comp;
        }

        // 첫 번째로 일치하는 타입의 컴포넌트를 반환(없으면 nullptr).
        template<typename T>
        std::shared_ptr<T> GetComponent()
        {
            for (auto& comp : m_components)
                if (auto p = std::dynamic_pointer_cast<T>(comp))
                    return p;
            return nullptr;
        }

        // 루트 트랜스폼(비소유 참조). 만료 시 nullptr.
        std::shared_ptr<SceneComponent> GetRoot() const { return m_root.lock(); }

        // 소유 컴포넌트 목록(비소유 관측용 — 에디터 인스펙터 등).
        const std::vector<std::shared_ptr<Component>>& GetComponents() const { return m_components; }

        // 표시용 이름(에디터 Hierarchy 등). 기본값 "Actor".
        void SetName(const std::string& _name) { m_name = _name; }
        const std::string& GetName() const { return m_name; }

    private:
        std::vector<std::shared_ptr<Component>> m_components;
        std::weak_ptr<SceneComponent> m_root;
        std::string m_name = "Actor";
    };
}
