#pragma once
#include <vector>
#include <memory>
#include <string>
#include <type_traits>
#include "Scene/Tag.h"
#include "Scene/SceneComponent.h"

namespace MiniEngine
{
    class Scene;
    namespace Graphics { struct RenderContext; }

    class Actor : public std::enable_shared_from_this<Actor>
    {
    public:
        virtual ~Actor() = default;

        virtual void BeginPlay() {}
        virtual void Tick(float _dt); // 소유 컴포넌트 Tick 전파
        virtual void Render(Graphics::RenderContext& _context);
        virtual void EndPlay() {}

        // 컴포넌트 추가
        template<typename T, typename... Args>
        std::shared_ptr<T> AddComponent(Args&&... _args);

        // 첫 번째로 일치하는 타입의 컴포넌트를 반환. 없으면 nullptr. 가급적 사용하지 말 것
        template<typename T>
        std::shared_ptr<T> GetComponent() const; 

        // 루트 트랜스폼(비소유 참조). 만료 시 nullptr.
        std::shared_ptr<SceneComponent> GetRoot() const { return m_root.lock(); }

        // 소유 컴포넌트 목록(비소유 관측용 — 에디터 인스펙터 등).
        const std::vector<std::shared_ptr<Component>>& GetComponents() const { return m_components; }

        // 표시용 이름(에디터 Hierarchy 등). 기본값 "Actor".
        void SetName(const std::string& _name) { m_name = _name; }
        const std::string& GetName() const { return m_name; }

        void OnSpawned(std::weak_ptr<Scene> _scene);
        std::shared_ptr<Scene> GetScene() const { return m_scene.lock(); };

        Tag& GetTag() { return m_tag; }

    private:
        Tag m_tag;

        std::weak_ptr<Scene> m_scene;
        std::vector<std::shared_ptr<Component>> m_components;
        std::weak_ptr<SceneComponent> m_root;
        std::string m_name = "Actor";
    };

    template<typename T, typename ...Args>
    inline std::shared_ptr<T> Actor::AddComponent(Args && ..._args)
    {
        // Component 요청한게 아니면 assert
        static_assert(std::is_base_of<Component, T>::value, "T must derive from Component");

        // owner 설정 
        std::shared_ptr<T> comp = std::make_shared<T>(std::forward<Args>(_args)...);
        comp->owner = weak_from_this(); // 약참조 의존성 주입

        // 첫 SceneComponent면 root 자동 지정 + OnAttach 호출.
        if (m_root.expired())
        {
            if (auto sc = std::dynamic_pointer_cast<SceneComponent>(comp))
                m_root = sc;
        }

        m_components.push_back(comp);
        comp->OnAttach();

        return comp;
    }

    template<typename T>
    inline std::shared_ptr<T> Actor::GetComponent() const
    {
        for (const std::shared_ptr<Component>& comp : m_components)
            if (std::shared_ptr<T> p = std::dynamic_pointer_cast<T>(comp))
                return p;

        return nullptr;
    }
}
