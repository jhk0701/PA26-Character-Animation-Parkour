#include "pch.h"
#include "Core/Log.h"
#include "Core/DebugMarkers.h"
#include "Core/Graphics.h"
#include "Scene/Scene.h"
#include "Scene/CameraComponent.h"
#include "Physics/PhysicsWorld.h"

namespace MiniEngine
{
    static constexpr float FIXED_DT = 1.0f / 60.0f;

    void Scene::Construct(ID3D11Device* _device, ID3D11DeviceContext* _context)
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
        m_light.m_dir = Vector3(-0.4f, -1.0f, -0.5f);
        m_light.m_dir.Normalize();
        m_light.m_ambient = 0.3f;
        m_light.m_color = Vector3(1.0f, 1.0f, 1.0f);

#if MG_DEBUG || MG_DEBUG_LOG || WITH_EDITOR
        m_debugDraw.Init(_device, _context);
#endif // DEBUG
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

        for (std::shared_ptr<Actor> pActor : GetActors())
            pActor->FixedTick(_dt);
    }

    void Scene::Tick(float _dt)
    {
        if (m_bMarkerDebug)
            MiniEngine::Debug::NewFrame(_dt);

        for (std::shared_ptr<Actor>& pActor : m_actors)
            pActor->Tick(_dt);
    }

    void Scene::LateTick(float _dt)
    {
        for (std::shared_ptr<Actor>& pActor : m_actors)
            pActor->LateTick(_dt);
    }

    void Scene::Render(Graphics::RenderContext& _context)
    {
        WriteCameraData(_context);
        WriteFrameCB(_context); // 라이트 등

        for (std::shared_ptr<Actor>& pActor : m_actors)
            pActor->Render(_context);

#if MG_DEBUG || MG_DEBUG_LOG || WITH_EDITOR
        if (m_bPhysicsDebug || m_bMarkerDebug)
        {
            m_debugLines.clear();

            if (m_bPhysicsDebug)
                m_physics->CollectDebugLines(m_debugLines);

            if (m_bMarkerDebug)
                MiniEngine::Debug::Collect(m_debugLines);
            
            std::shared_ptr<CameraComponent> mainCam = GetMainCamera().lock();
            m_debugDraw.Draw(m_debugLines, mainCam->GetViewMatrix() * mainCam->GetProjectionMatrix());
        }
#endif 
    }

    void Scene::EndPlay()
    {
        for (std::shared_ptr<Actor>& pActor : m_actors)
            pActor->EndPlay();

        m_physics->Shutdown();
    }

    void Scene::ApplyMarkerDebug(bool _enable)
    {
        if (m_bMarkerDebug == _enable)
            return;

        m_bMarkerDebug = _enable;

        MG_LOG_INFO("[Scene]::Apply Marker Debug {}", _enable ? "On" : "Off");
    }

    void Scene::ApplyPhysicsDebug(bool _enable)
    {
        if (m_bPhysicsDebug == _enable)
            return;

        m_bPhysicsDebug = _enable;
        
        m_physics->SetDebugVisualization(_enable);
        m_physics->SetDrawQueries(_enable);
        MG_LOG_INFO("[Scene]::Apply Physics Debug {}", _enable ? "On" : "Off");
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
