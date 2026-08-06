#pragma once
#include "Core/Math.h"
#include <vector>
#include "Core/DebugLine.h"
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
    class PxGeometry;
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

    struct RaycastParam
    {
        Vector3 m_origin;
        Vector3 m_dir;
        float m_maxDistance;
        // physx::PxHitFlag::Enum m_hitFlag{ physx::PxHitFlag::eDEFAULT };
        // physx::PxFilterData m_filterData; // 충돌할 대상 static, dynamic, any
    };

    struct HitResult
    {
        Vector3 m_pos;
        Vector3 m_nrm;
        float m_distance;
        physx::PxRigidActor* m_hitActor;
        physx::PxShape* m_hitShape;
        void* GetActor() const;
    };

    struct RaycastResult // 레이캐스트 후 결과
    {
        bool m_bIsHit{ false };
        Vector3 m_pos;
        Vector3 m_nrm;
        float m_distance;
        physx::PxRigidActor* m_hitActor;
        physx::PxShape* m_hitShape;

        void* GetActor() const;
        void FillFromHitResult(const bool _bIsHit, const HitResult& _hit);
    };


    struct RaycastMultipleResult
    {
        bool m_bIsHit{ false };
        std::vector<HitResult> m_hitResults;
    };


    struct SweepCommonParam 
    {
        float m_maxDistance{ 1.0f };
        Vector3 m_startPos{ 0.0f, 0.0f,0.0f };     // origin
        Quaternion m_startRot{ 0.0f, 0.0f, 0.0f, 1.0f };  // 회전각
        Vector3 m_dir;
    };
    struct SpherecastParam : public SweepCommonParam
    {
        float m_radius;
    };
    struct CapsulecastParam : public SweepCommonParam
    {
        CapsulecastParam() 
        {
            // 강제로 y축을 향해 일어나도록 조정
            m_startRot = Quaternion::CreateFromYawPitchRoll(0.0f, 0.0f, ToRadians(90.0f));
        }

        float m_radius{0.0f};
        float m_halfHeight{0.0f};
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

        bool Raycast(const RaycastParam& _inParam, RaycastResult& _outResult, uint32_t _layerMask = LayerMask::ALL) const;
        bool CapsuleCast(const CapsulecastParam& _inParam, RaycastResult& _outResult, uint32_t _layerMask = LayerMask::ALL) const;
        bool SphereCast(const SpherecastParam& _inParam, RaycastResult& _outResult, uint32_t _layerMask = LayerMask::ALL) const;
        bool CapsuleCastMultiple(const CapsulecastParam& _inParam, RaycastMultipleResult& _outResult, uint32_t _layerMask = LayerMask::ALL) const;
        bool SphereCastMultiple(const SpherecastParam& _inParam, RaycastMultipleResult& _outResult, uint32_t _layerMask = LayerMask::ALL) const;

        static void SetQueryLayer(physx::PxRigidActor& _actor, uint32_t _layerMask);
        static void SetCollisionLayer(physx::PxRigidActor& _actor, uint32_t _layerMask);

        void SetDebugVisualization(bool _enable, float _scale = 1.0f);
        void SetDrawQueries(bool _enable);
        void CollectDebugLines(std::vector<DebugLine>& _out);

    private:
        physx::PxFoundation* m_foundation = nullptr;
        physx::PxPhysics* m_physics = nullptr;
        physx::PxDefaultCpuDispatcher* m_dispatcher = nullptr;
        physx::PxScene* m_scene = nullptr;
        physx::PxMaterial* m_material = nullptr; // m_physics 소유(함께 해제)
        physx::PxControllerManager* m_controllerManager = nullptr; // 자신이 만든 PxController 를 모두 소유 — scene 보다 먼저 release

        bool m_drawQueries = false;
        mutable std::vector<DebugLine> m_queryLines;

        void RecordQueryLine(const Vector3& _origin, const Vector3& _unitDir, float _maxDistance,
            bool _hit, const RaycastResult& _hitInfo) const;

        bool SweepGeometry(const physx::PxGeometry& _inGeo, const SweepCommonParam& _inParam, RaycastResult& _outResult, uint32_t _layerMask = LayerMask::ALL) const;
        bool SweepGeometryMulti(const physx::PxGeometry& _inGeo, const SweepCommonParam& _inParam, RaycastMultipleResult& _outResult, uint32_t _layerMask = LayerMask::ALL) const;
    };


}