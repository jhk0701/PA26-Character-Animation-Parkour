#pragma once
#include <vector>
#include <memory>
#include <type_traits>
#include <utility>
#include <d3d11.h>

#include "Scene/Actor.h"
#include "Physics/PhysicsWorld.h"

// 콜라이더 및 레이캐스트 디버깅용
#include "Core/DebugLine.h"
#include "Editor/DebugDraw.h"

namespace MiniEngine
{
    namespace Graphics { struct RenderContext; }

    struct Light // 렌더링용 간이 디렉셔널 라이트
    {
        Vector3 m_dir;
        Vector3 m_color;
        float m_ambient;
        Vector3 m_albedo;
    };

    class CameraComponent;
    class Scene : public std::enable_shared_from_this<Scene>
    {
    public:
        Scene() {};
        virtual ~Scene() {};

        template<typename T = Actor, typename... Args>
        std::shared_ptr<T> SpawnActor(Args&&... _args)
        {
            static_assert(std::is_base_of<Actor, T>::value, "T must derive from Actor");
            std::shared_ptr<T> actor = std::make_shared<T>(std::forward<Args>(_args)...);
            m_actors.push_back(actor);

            actor->OnSpawned(shared_from_this());

            return actor;
        }

        virtual void Construct(ID3D11Device* _device, ID3D11DeviceContext* _context);
        virtual void BeginPlay();
        virtual void FixedTick(float _dt);
        virtual void Tick(float _dt);
        virtual void Render(Graphics::RenderContext& _context);
        virtual void EndPlay();

        // setter
        void SetMainCamera(const std::shared_ptr<CameraComponent>& _newCamera) { m_mainCam = _newCamera; };
        
        // getter
        std::weak_ptr<CameraComponent> GetMainCamera() const { return m_mainCam; }
        const std::vector<std::shared_ptr<Actor>>& GetActors() const { return m_actors; }
        std::weak_ptr<Physics::PhysicsWorld> GetPhysics() const { return m_physics; };

        void ApplyPhysicsDebug(bool _enable);

    protected:
        Light& GetLight() { return m_light; }

    private:
        // 관리할 액터 인스턴스
        std::vector<std::shared_ptr<Actor>> m_actors;

        // 물리 엔진
        std::shared_ptr<Physics::PhysicsWorld> m_physics;   // 씬마다 설치해둘 것
        float m_physicsAcuum{ 0.0f };                       // 고정 60Hz로 스텝할 것

        // 씬 렌더링
        Light m_light;
        std::shared_ptr<CameraComponent> m_defaultCam; // m_mainCam이 없을때 사용할 원점 위치의 카메라
        std::shared_ptr<CameraComponent> m_mainCam;

        void WriteCameraData(Graphics::RenderContext& _outContext);
        void WriteFrameCB(Graphics::RenderContext& _outContext);

        // 물리엔진 디버깅 렌더러
        bool m_PhysicsDebug{ false };
        MiniEngine::DebugDrawer m_debugDraw;
        std::vector<MiniEngine::DebugLine> m_debugLines;
    };
}
