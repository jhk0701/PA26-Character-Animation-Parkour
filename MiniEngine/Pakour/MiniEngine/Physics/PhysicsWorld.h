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
    };
}