#pragma once
#include "Core/Math.h"

namespace physx
{
    class PxFoundation;
    class PxPhysics;
    class PxScene;
    class PxMaterial;
    class PxDefaultCpuDispatcher;
    class PxRigidActor;
}

namespace MiniEngine::Physics 
{
    // 충돌을 감지할 그룹 종류 (레이어)
    // 레이캐스트 쿼리와는 별개임
    enum ECollisionGroup : uint16_t
    {
        Player,
        
        Land,       // 착지할 공간
        Obstacle,   // 액터가 지나가지 못하는 장애물
        
        IgnoreAll,
    };


    class PhysicsWorld
    {
    public:
        PhysicsWorld();
        ~PhysicsWorld();

        PhysicsWorld(const PhysicsWorld&) = delete;
        PhysicsWorld& operator=(const PhysicsWorld&) = delete;

        bool Init();
        void Shutdown();

        void Step(float _fixedDt);
        bool IsInitialized() const { return m_scene != nullptr; }

        bool CreateRigidFloor();
        physx::PxRigidActor* CreateDynamicBox(const Vector3& _pos, const Quaternion& _rot, const Vector3& _halfExtents, float _density);
        physx::PxRigidActor* CreateStaticBox(const Vector3& _pos, const Quaternion& _rot, const Vector3& _halfExtents);

    private:
        physx::PxFoundation* m_foundation = nullptr;
        physx::PxPhysics* m_physics = nullptr;
        physx::PxDefaultCpuDispatcher* m_dispatcher = nullptr;
        physx::PxScene* m_scene = nullptr;
        physx::PxMaterial* m_material = nullptr; // m_physics 소유(함께 해제)

        void SetDefaultSimulateFilter();
    };
}