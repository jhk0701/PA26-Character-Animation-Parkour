#pragma once
#include <vector>
#include <memory>
#include <type_traits>
#include <utility>

#include "Scene/Actor.h"
#include "Physics/PhysicsWorld.h"

namespace MiniEngine
{
    class CameraComponent;
    class World
    {
    public:
        World() {};
        virtual ~World() {};

        template<typename T = Actor, typename... Args>
        std::shared_ptr<T> SpawnActor(Args&&... _args)
        {
            static_assert(std::is_base_of<Actor, T>::value, "T must derive from Actor");
            auto actor = std::make_shared<T>(std::forward<Args>(_args)...);
            m_actors.push_back(actor);
            return actor;
        }

        virtual void Construct() = 0;
        virtual void BeginPlay();
        virtual void Tick(float _dt);
        virtual void Render();
        virtual void EndPlay();

        const std::vector<std::shared_ptr<Actor>>& GetActors() const { return m_actors; }

        void SetMainCamera(const std::shared_ptr<CameraComponent>& _newCamera) { m_mainCam = _newCamera; };
        std::weak_ptr<CameraComponent> GetMainCamera() const { return m_mainCam; }

    private:
        std::shared_ptr<CameraComponent> m_mainCam;
        std::vector<std::shared_ptr<Actor>> m_actors;
        
        // 물리 엔진 파트 : 
        Physics::PhysicsWorld m_physics;
        float m_physicsAcuum = 0.0f; // 고정 60Hz로 스텝할 것
    };
}
