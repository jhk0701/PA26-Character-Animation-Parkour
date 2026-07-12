#pragma once
#include "Core/Math.h"
#include "Physics/CollsionLayer.h"

namespace physx
{
    class PxFoundation;
    class PxPhysics;
    class PxScene;
    class PxMaterial;
    class PxDefaultCpuDispatcher;
    class PxRigidActor;
    class PxShape;
    class PxController;
    class PxControllerManager;
}

namespace MiniEngine::Physics 
{
    struct CapsuleControllerDesc
    {
        Vector3 footPosition{ 0.0f, 0.0f, 0.0f }; // 캡슐 발 위치
        float radius = 0.04f;
        float height = 1.0f;
        float stepOffset = 0.3f; // 바닥에 무시할 수 있는 턱의 높이
        float contactOffset = 0.1f; // 수치 안정용
        float slopeLimitDeg = 45.0f; // 걸어 올라갈 수 있는 경사각
    };

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
        
        physx::PxController* CreateCapsuleController(const CapsuleControllerDesc& _desc);

        void ToggleDebugMode(bool _bIsOn);

        bool Raycast(const RaycastParam& _inParam, RaycastResult& _outResult, uint32_t _layerMask = LayerMask::ALL) const;
        static void SetQueryLayer(physx::PxRigidActor& _actor, uint32_t _layerMask);

    private:
        bool m_bIsDebugging{ false };

        physx::PxFoundation* m_foundation = nullptr;
        physx::PxPhysics* m_physics = nullptr;
        physx::PxDefaultCpuDispatcher* m_dispatcher = nullptr;
        physx::PxScene* m_scene = nullptr;
        physx::PxMaterial* m_material = nullptr; // m_physics 소유(함께 해제)
        physx::PxControllerManager* m_controllerManager = nullptr; // 자신이 만든 PxController 를 모두 소유 — scene 보다 먼저 release

        void SetDefaultCollisionGroup();
    };


}