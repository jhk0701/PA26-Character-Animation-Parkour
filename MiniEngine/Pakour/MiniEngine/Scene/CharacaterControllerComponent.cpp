#include "pch.h"
#include "Scene/CharacaterControllerComponent.h"
#include "Scene/Actor.h"
#include "Scene/SceneComponent.h"
#include "Scene/SkeletalMeshComponent.h"
#include "Core/Log.h"
#include <physx/PxPhysicsAPI.h>

namespace MiniEngine
{
	void CharacaterControllerComponent::CheckParented(const std::shared_ptr<SceneComponent>& _target)
	{
		// RigidBody 처럼 Global Transform이어야 하므로 확인 코드 추가
		if (_target && _target->GetParent())
			MG_LOG_WARN("[CharacaterControllerComponent] target에 부모가 있음");
	}

	void CharacaterControllerComponent::Init(Physics::PhysicsWorld& _world, 
		const Physics::CapsuleControllerDesc& _desc, const std::shared_ptr<SceneComponent>& _target)
	{
		std::shared_ptr<SceneComponent> target = _target;
		if (!target) 
		{
			if (std::shared_ptr<Actor> o = owner.lock())
				target = o->GetRoot();
		}

		if (!target) 
		{
			MG_LOG_ERROR("[CharacterControllerComp::Init] target SceneComponent가 유효하지 않음");
			return;
		}

		CheckParented(target);

		// 캡슐 발 위치 맞추기 : 현재 액터의 위치로
		Physics::CapsuleControllerDesc desc = _desc;
		desc.footPosition = target->localTransform.position;

		m_controller = _world.CreateCapsuleController(desc);
		if (!m_controller)
			return;

		m_target = target;

		// userdata 설정
		std::shared_ptr<Actor> o = owner.lock();
		if (physx::PxRigidDynamic* actor = m_controller->getActor())
			actor->userData = o.get();
	}

	void CharacaterControllerComponent::AddMovementInput(const Vector3& _worldDelta)
	{
		m_pendingMove += _worldDelta;
	}

	void CharacaterControllerComponent::AddYaw(const Quaternion& _deltaRotation)
	{
		std::shared_ptr<SceneComponent> target = m_target.lock();
		if (!target)
			return;

		target->localTransform.rotation = _deltaRotation * target->localTransform.rotation;
		target->localTransform.rotation.Normalize();
	}

	void CharacaterControllerComponent::Jump(float _speed)
	{
		if (!m_grounded)
			return;

		m_verticalVelocity = _speed;
		m_grounded = false;
	}

	void CharacaterControllerComponent::Move(float _dt)
	{
		std::shared_ptr<SceneComponent> target = m_target.lock();
		if (!m_controller || !target)
			return;

		// 루트모션 적용
		if (std::shared_ptr<SkeletalMeshComponent> sk = m_rootMotionSource.lock()) 
		{
			const RootMotionDelta d = sk->ConsumeRootMotionDelta();

			// 회전 갱신 전 회전값으로 먼저 translation 변환
			// 루트모션은 기본적인 이동 이후에 회전할 것
			const Vector3 scaled = d.translation * target->localTransform.scale * sk->localTransform.scale;
			m_pendingMove += Vector3::Transform(scaled, target->localTransform.rotation);

			target->localTransform.rotation = d.rotation * target->localTransform.rotation;
			target->localTransform.rotation.Normalize();
		}

		// 중력 가속도 계산 적용
		m_verticalVelocity == m_gravity * _dt;
		if (m_grounded && m_verticalVelocity < 0.0f)
			m_verticalVelocity = -STICK_TO_GROUND_SPEED; // 지면에 계속 붙여 eCollsion_down 유지

		// 이동
		const Vector3 disp = m_pendingMove + Vector3(0.0f, m_verticalVelocity * _dt, 0.0f);
		const physx::PxControllerCollisionFlags flags = m_controller->move(
			physx::PxVec3(disp.x, disp.y, disp.z), 0.001f, _dt, physx::PxControllerFilters()
		);

		m_grounded = flags.isSet(physx::PxControllerCollisionFlag::eCOLLISION_DOWN);
		if (m_grounded && m_verticalVelocity < 0.0f)
			m_verticalVelocity = 0.0f; // 착지한 경우, vertical velocity 초기화
		if (flags.isSet(physx::PxControllerCollisionFlag::eCOLLISION_UP) && m_verticalVelocity > 0.0f)
			m_verticalVelocity = 0.0f; // 천장 등등 머리가 접촉한 경우, 상승 초기화

		m_pendingMove = Vector3(0.0f);
	}

	void CharacaterControllerComponent::SyncTransform()
	{
		std::shared_ptr<SceneComponent> target = m_target.lock();
		if (!m_controller || !target)
			return;

		// 초기화 시 설정한 발의 위치를 기준으로 transform 적용
		const physx::PxVec3 foot = physx::toVec3(m_controller->getFootPosition());
		target->localTransform.position = Vector3(foot.x, foot.y, foot.z);
	}

	void CharacaterControllerComponent::SetRootMotionSource(const std::shared_ptr<SkeletalMeshComponent>& _skeletal)
	{
		m_rootMotionSource = _skeletal;
		if (!_skeletal)
			return;

		if (!_skeletal->IsRootMotionEnabled())
			MG_LOG_WARN("[CharactorControllerComp] 소스에 RootMotionEnable이 true가 아님");

		const Quaternion& r = _skeletal->localTransform.rotation;
		if (fabsf(fabsf(r.w) - 1.0f) > 1e-4f)
			MG_LOG_WARN("[CharactorControllerComp] 루트 모션 소스의 local 회전이 identity가 아님 -> 이동 방향이 어긋날 것");
	}

	Vector3 CharacaterControllerComponent::GetFootPosition() const
	{
		if (!m_controller)
			return Vector3(0.0f);

		const physx::PxVec3 foot = physx::toVec3(m_controller->getFootPosition());
		return Vector3(foot.x, foot.y, foot.z);
	}
}