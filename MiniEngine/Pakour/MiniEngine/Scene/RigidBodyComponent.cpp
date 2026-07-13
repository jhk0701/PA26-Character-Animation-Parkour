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

	void RigidBodyComponent::Init(Physics::PhysicsWorld& _world, EBodyType _type, const Vector3& _halfExtents, float _denity, const std::shared_ptr<SceneComponent>& _target)
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

		const Vector3& pos = target->localTransform.position;
		const Quaternion& rot = target->localTransform.rotation;

		m_actor = (_type == EBodyType::Dynamic) ?
			_world.CreateDynamicBox(pos, rot, _halfExtents, _denity) :
			_world.CreateStaticBox(pos, rot, _halfExtents);

		// UserData로 owner 액터 포인터 적용
		if (std::shared_ptr<Actor> o = owner.lock())
			m_actor->userData = o.get();
	}

	//void RigidBodyComponent::SetCollsionGroup(Physics::ECollisionGroup _group)
	//{
	//	physx::PxSetGroup(*m_actor, _group);
	//}

	//Physics::ECollisionGroup RigidBodyComponent::GetCollsionGroup() const
	//{
	//	return (Physics::ECollisionGroup)physx::PxGetGroup(*m_actor);
	//}

	void RigidBodyComponent::SyncTransform()
	{
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