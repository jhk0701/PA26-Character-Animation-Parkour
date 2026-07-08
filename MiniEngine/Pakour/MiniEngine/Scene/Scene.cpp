#include "pch.h"
#include "Core/Log.h"
#include "Core/Graphics.h"
#include "Scene/Scene.h"
#include "Scene/CameraComponent.h"

// 테스트용 임시 추가
#include "Scene/RigidBodyComponent.h"

namespace MiniEngine
{
    static constexpr float FIXED_DT = 1.0f / 60.0f;

    void Scene::Construct()
    {
        m_physics = std::make_shared<Physics::PhysicsWorld>();
        if (m_physics && m_physics->Init() == false)
        {
            MG_LOG_ERROR("Scene : PhysX Init failed");
            return;
        }

        // main cam이 생성되지 않을때를 대비한 기본 카메라 컴포넌트
        m_defaultCam = std::make_shared<CameraComponent>();
        m_defaultCam->aspect = static_cast<float>(WINDOW_WIDTH) / static_cast<float>(WINDOW_HEIGHT);
        m_defaultCam->localTransform.position = Vector3(0.0f);

        // 라이트 기본값
        m_light.m_dir = Vector3(-0.4f, -1.0f, 0.6f);
        m_light.m_dir.Normalize();
        m_light.m_ambient = 0.15f;
        m_light.m_color = Vector3(1.0f, 1.0f, 1.0f);
    }

    void Scene::BeginPlay()
    {
        for (std::shared_ptr<Actor>& actor : m_actors)
            actor->BeginPlay();
    }

    void Scene::FixedTick(float _dt)
    {
        m_physicsAcuum += _dt;

        while (m_physicsAcuum >= FIXED_DT) 
        {
            m_physics->Step(FIXED_DT);
            m_physicsAcuum -= FIXED_DT;
        }

        // TODO : 물리 강체 관련해서 억지로 조정중 -> 컴포넌트 위계를 통한 보정으로 변경하기
        for (std::shared_ptr<Actor> pActor : GetActors())
        {
            if (std::shared_ptr<RigidBodyComponent> pRB = pActor->GetComponent<RigidBodyComponent>())
                pRB->SyncTransform();
        }
    }

    void Scene::Tick(float _dt)
    {
        for (std::shared_ptr<Actor>& actor : m_actors)
            actor->Tick(_dt);
    }

    void Scene::Render(Graphics::RenderContext& _context)
    {
        WriteCameraData(_context);
        WriteFrameCB(_context); // 라이트 등

        for (std::shared_ptr<Actor>& actor : m_actors)
            actor->Render(_context);
    }

    void Scene::EndPlay()
    {
        for (std::shared_ptr<Actor>& actor : m_actors)
            actor->EndPlay();

        m_physics->Shutdown();
    }

    void Scene::WriteCameraData(Graphics::RenderContext& _outContext)
    {
        // 렌더링 컨텍스트에 현재 카메라 정보 기입
        if (!m_mainCam)
        {
            _outContext.m_camView = m_defaultCam->GetViewMatrix();
            _outContext.m_camProj = m_defaultCam->GetProjectionMatrix();
            return;
        }

        _outContext.m_camView = m_mainCam->GetViewMatrix();
        _outContext.m_camProj = m_mainCam->GetProjectionMatrix();
    }

    void Scene::WriteFrameCB(Graphics::RenderContext& _outContext)
    {
        _outContext.m_perFrame.lightDir     = m_light.m_dir;
        _outContext.m_perFrame.ambient      = m_light.m_ambient;
        _outContext.m_perFrame.lightColor   = m_light.m_color;
        _outContext.m_perFrame.albedo       = m_light.m_albedo;
    }
}
