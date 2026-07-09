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
    class PxShape;
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
        
        IgnoreAll = 31,
        END = 32
    };

    struct RaycastParam 
    {
        Vector3 m_origin;
        Vector3 m_dir;
        float m_maxDistance;
        // physx::PxHitFlag::Enum m_hitFlag{ physx::PxHitFlag::eDEFAULT };
        // physx::PxFilterData m_filterData; // 충돌할 대상 static, dynamic, any
    };

    struct RaycastResult // 레이캐스트 후 결과
    {
        Vector3 m_pos;
        Vector3 m_nrm;
        float m_distance;
        physx::PxRigidActor* m_hitActor;
        physx::PxShape* m_hitShape;

        void* GetActor() const;
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
        physx::PxRigidActor* CreateStaticBox(const Vector3& _pos, const Quaternion& _rot, const Vector3& _halfExtents);
        physx::PxRigidActor* CreateDynamicBox(const Vector3& _pos, const Quaternion& _rot, const Vector3& _halfExtents, float _density);
        physx::PxRigidActor* CreateDynamicCapsule(const Vector3& _pos, const Quaternion& _rot, float _radius, float _height, float _density);

        void ToggleDebugMode(bool _bIsOn);

        bool Raycast(const RaycastParam& _inParam, RaycastResult& _outResult);

    private:
        bool m_bIsDebugging{ false };

        physx::PxFoundation* m_foundation = nullptr;
        physx::PxPhysics* m_physics = nullptr;
        physx::PxDefaultCpuDispatcher* m_dispatcher = nullptr;
        physx::PxScene* m_scene = nullptr;
        physx::PxMaterial* m_material = nullptr; // m_physics 소유(함께 해제)

        void SetDefaultCollisionGroup();
    };


}