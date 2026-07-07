#include "pch.h"
#include "Core/Log.h"
#include "Scene/World.h"

// 테스트용 임시 추가
#include "Scene/RigidBodyComponent.h"

namespace MiniEngine
{
    static constexpr float FIXED_DT = 1.0f / 60.0f;

    void World::Construct()
    {
        m_physics = std::make_shared<Physics::PhysicsWorld>();
        if (m_physics && m_physics->Init() == false)
        {
            MG_LOG_ERROR("World : PhysX Init failed");
            return;
        }
    }

    void World::BeginPlay()
    {
        for (std::shared_ptr<Actor>& actor : m_actors)
            actor->BeginPlay();
    }

    void World::FixedTick(float _dt)
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

    void World::Tick(float _dt)
    {
        for (std::shared_ptr<Actor>& actor : m_actors)
            actor->Tick(_dt);
    }

    void World::Render()
    {
        for (std::shared_ptr<Actor>& actor : m_actors)
            actor->Render();
    }

    void World::EndPlay()
    {
        for (std::shared_ptr<Actor>& actor : m_actors)
            actor->EndPlay();

        m_physics->Shutdown();
    }
}
