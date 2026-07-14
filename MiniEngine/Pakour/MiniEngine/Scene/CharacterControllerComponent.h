#pragma once
#include <memory>
#include "Scene/Component.h"
#include "Physics/PhysicsWorld.h"
#include "Core/Math.h"
#include "Physics/CollsionLayer.h"

namespace physx { class PxController; }

namespace MiniEngine
{
	class SceneComponent;
    class SkeletalMeshComponent;
	class CharacterControllerComponent : public Component
	{
	public:
        void FixedTick(float _dt) override;

		void Init(Physics::PhysicsWorld& _world, const Physics::CapsuleControllerDesc& _desc,
			const std::shared_ptr<SceneComponent>& _target = nullptr);

        void AddMovementInput(const Vector3& _worldDelta);
        void AddYaw(const Quaternion& _deltaRotation);
        
        void Jump(float _speed);
        void Move(float _dt);
        
        void SyncTransform();
        void SetRootMotionSource(const std::shared_ptr<SkeletalMeshComponent>& _skeletal);

        bool IsFalling() const { return m_bIsFalling; }
        bool IsGrounded() const { return m_grounded; }
        float GetVerticalVelocity() const { return m_verticalVelocity; }
        Vector3 GetFootPosition() const;

        void SetGravity(float _gravity) { m_gravity = _gravity; }
        std::shared_ptr<SceneComponent> GetTarget() const { return m_target.lock(); }

        void SetQueryLayer(Physics::Layer _layer);
        
        void SetCollisionMask(uint32_t _mask);
        // _enable = false -> 충돌하지 않도록 설정
        void SetLayerCollisionEnabled(Physics::Layer _layer, bool _enabled);

        void SetFallingSecondThreshold(float _sec) { m_fallingSecThreshold = _sec; };
        /*
        void SetCollsionGroup(Physics::ECollisionGroup _group);
        Physics::ECollisionGroup GetCollsionGroup() const;
        */

    private:
        void CheckParented(const std::shared_ptr<SceneComponent>& _target);
        void CheckFalling(float _fixedDt);

        physx::PxController* m_controller = nullptr;
        std::weak_ptr<SceneComponent> m_target;
        std::weak_ptr<SkeletalMeshComponent> m_rootMotionSource;
       
        uint32_t m_collisionMask = Physics::LayerMask::ALL;
        Vector3 m_pendingMove{ 0.0f, 0.0f, 0.0f };
        float   m_gravity = -9.81f;
        float   m_verticalVelocity = 0.0f;
        bool    m_grounded = false;

        static constexpr float STICK_TO_GROUND_SPEED = 2.0f;

        bool    m_bIsFalling{ false };
        float   m_fallingSecThreshold{ 0.3f }; // 일정시간 동안 지속적으로 떨어져야 낙하인정
        float   m_fallingElapsed{ 0.0f };

        
	};
}