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

        bool IsGrounded() const { return m_grounded; }
        float GetVerticalVelocity() const { return m_verticalVelocity; }
        Vector3 GetFootPosition() const;

        void SetGravity(float _gravity) { m_gravity = _gravity; }
        std::shared_ptr<SceneComponent> GetTarget() const { return m_target.lock(); }

        void SetLayer(Physics::Layer _layer);

    private:
        void CheckParented(const std::shared_ptr<SceneComponent>& _target);

        physx::PxController* m_controller = nullptr;
        std::weak_ptr<SceneComponent> m_target;
        std::weak_ptr<SkeletalMeshComponent> m_rootMotionSource;

        Vector3 m_pendingMove{ 0.0f, 0.0f, 0.0f };
        float   m_gravity = -9.81f;
        float   m_verticalVelocity = 0.0f;
        bool    m_grounded = false;

        static constexpr float STICK_TO_GROUND_SPEED = 2.0f;
	};
}