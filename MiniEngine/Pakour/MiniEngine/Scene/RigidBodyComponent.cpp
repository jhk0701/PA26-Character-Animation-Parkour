#include "pch.h"
#include "Scene/RigidBodyComponent.h"
#include "Scene/Actor.h"
#include "Core/Log.h"

#include <physx/PxPhysicsAPI.h>
#include "Physics/PhysicsWorld.h"
#include "CharacterControllerComponent.h"

namespace MiniEngine
{
	void RigidBodyComponent::FixedTick(float _dt)
	{
		Component::FixedTick(_dt);
		
		SyncTransform();
	}

	void RigidBodyComponent::Init(Physics::PhysicsWorld& _world, 
		EBodyType _type, 
		const Vector3& _halfExtents, 
		const std::shared_ptr<SceneComponent>& _target,
		const std::shared_ptr<SceneComponent>& _parent,
		float _denity, 
		bool _bIsSub)
	{
		// root 컴포넌트를 대상으로 Rigidbody가 Transform을 사용할 것
		std::shared_ptr<SceneComponent> target = _target;
		if (target == nullptr) 
		{
			if  (std::shared_ptr<Actor> o = owner.lock())
				target = o->GetRoot();
		}

		if (target == nullptr)
		{
			MG_LOG_ERROR("[RigidBodyComponent::Init] target SceneComponent가 유효하지 않음");
			return;
		}
		
		CheckParented(target);

		m_type = _type;
		m_target = target;
		m_bIsSub = _bIsSub;

		Vector3 pos(0.0f);
		
		if (_parent) 
		{
			const Transform& TF = _parent->localTransform;
			pos = TF.position;
			pos += target->localTransform.position.x * TF.Right();
			pos += target->localTransform.position.y * TF.Up();
			pos += target->localTransform.position.z * TF.Forward();
		}
		else 
			pos = target->localTransform.position;

		Quaternion rot = _parent ?
			_parent->localTransform.rotation * target->localTransform.rotation : 
			target->localTransform.rotation;

		m_actor = (_type == EBodyType::Dynamic) ?
			_world.CreateDynamicBox(pos, rot, _halfExtents, _denity) :
			_world.CreateStaticBox(pos, rot, _halfExtents);

		// UserData로 owner 액터 포인터 적용
		if (std::shared_ptr<Actor> o = owner.lock())
			m_actor->userData = reinterpret_cast<void*>(o.get());
	}

	void RigidBodyComponent::SyncTransform()
	{
		// 메인인 경우에만 적용
		// 메인 충돌 액터만 위치를 반영하고
		// 서브 리지드 바디들은 변경된 위치를 내려받음
		if (m_bIsSub)
			return;  

		if (m_type != EBodyType::Dynamic || !m_actor)
			return;

		std::shared_ptr<SceneComponent> target = m_target.lock();
		if (!target)
			return;

		const physx::PxTransform t = m_actor->getGlobalPose();
		target->localTransform.position = Vector3(t.p.x, t.p.y, t.p.z);
		target->localTransform.rotation = Quaternion(t.q.x, t.q.y, t.q.z, t.q.w);
	}

	void RigidBodyComponent::PushTransform()
	{
		if (!m_actor)
			return;

		std::shared_ptr<SceneComponent> target = m_target.lock();
		if (!target)
			return;

		CheckParented(target);

		const Transform& t = target->localTransform;
		m_actor->setGlobalPose(physx::PxTransform(
			physx::PxVec3(t.position.x, t.position.y, t.position.z),
			physx::PxQuat(t.rotation.x, t.rotation.y, t.rotation.z, t.rotation.w)
		));

		if (m_type == EBodyType::Dynamic)
		{
			physx::PxRigidDynamic* dynamicActor = static_cast<physx::PxRigidDynamic*>(m_actor);
			dynamicActor->setLinearVelocity(physx::PxVec3(0.0f));
			dynamicActor->setAngularVelocity(physx::PxVec3(0.0f));
		}
	}

	void RigidBodyComponent::SetQueryLayer(Physics::Layer _layer)
	{
		if (!m_actor)
			return;

		Physics::PhysicsWorld::SetQueryLayer(*m_actor, Physics::ToMask(_layer));
	}

	void RigidBodyComponent::SetCollisionLayer(Physics::Layer _layer)
	{
		if (!m_actor)
			return;

		Physics::PhysicsWorld::SetCollisionLayer(*m_actor, Physics::ToMask(_layer));
	}

	void RigidBodyComponent::CheckParented(const std::shared_ptr<SceneComponent>& _target)
	{
		if (_target && _target->GetParent())
			MG_LOG_WARN("[RigidBodyComp] target에 부모가 있음");
	}
	
}