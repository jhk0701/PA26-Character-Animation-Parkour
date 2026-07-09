#include "pch.h"
#include "Scene/RigidBodyComponent.h"
#include "Scene/Actor.h"

#include <physx/PxPhysicsAPI.h>
#include "Physics/PhysicsWorld.h"

namespace MiniEngine
{
	void RigidBodyComponent::Init(Physics::PhysicsWorld& _world, EBodyType _type, const Vector3& _halfExtents, float _denity)
	{
		if (owner.expired())
			return;

		m_type = _type;
		std::shared_ptr<Actor> pOwner = owner.lock();
		std::shared_ptr<SceneComponent> pRoot = pOwner->GetRoot();

		if (!pRoot)
			return;

		Vector3 pos(0.0f);
		Quaternion rot(0.0f, 0.0f, 0.0f, 1.0f);

		pos = pRoot->localTransform.position;
		rot = pRoot->localTransform.rotation;

		m_actor = (_type == EBodyType::Dynamic) ?
			_world.CreateDynamicBox(pos, rot, _halfExtents, _denity) :
			_world.CreateStaticBox(pos, rot, _halfExtents);

		// UserData로 owner 액터 포인터 적용
		m_actor->userData = reinterpret_cast<void*>(pOwner.get());
	}

	void RigidBodyComponent::InitDynamicCapsule(Physics::PhysicsWorld& _world, const Vector2& _capsuleExtent, float _denity)
	{
		if (owner.expired())
			return;

		m_type = EBodyType::Dynamic;
		std::shared_ptr<Actor> pOwner = owner.lock();
		std::shared_ptr<SceneComponent> pRoot = pOwner->GetRoot();

		if (!pRoot)
			return;

		Vector3 pos(0.0f);
		Quaternion rot(0.0f, 0.0f, 0.0f, 1.0f);

		pos = pRoot->localTransform.position;
		rot = pRoot->localTransform.rotation;

		pos.y -= _capsuleExtent.y;
		m_actor = _world.CreateDynamicCapsule(pos, rot, _capsuleExtent.x, _capsuleExtent.y, _denity);

		// UserData로 owner 액터 포인터 적용
		m_actor->userData = reinterpret_cast<void*>(pOwner.get());
	}

	void RigidBodyComponent::SetCollsionGroup(Physics::ECollisionGroup _group)
	{
		physx::PxSetGroup(*m_actor, _group);
	}

	Physics::ECollisionGroup RigidBodyComponent::GetCollsionGroup() const
	{
		return (Physics::ECollisionGroup)physx::PxGetGroup(*m_actor);
	}

	void RigidBodyComponent::SyncTransform()
	{
		if (m_type != EBodyType::Dynamic || !m_actor)
			return;

		const physx::PxTransform t = m_actor->getGlobalPose();
		if (owner.expired())
			return;

		std::shared_ptr<Actor> pOwner = owner.lock();
		std::shared_ptr<SceneComponent> pRoot = pOwner->GetRoot();

		if (!pRoot)
			return;

		pRoot->localTransform.position = Vector3(t.p.x, t.p.y, t.p.z);
		pRoot->localTransform.rotation = Quaternion(t.q.x, t.q.y, t.q.z, t.q.w);
	}
	
	void RigidBodyComponent::SetKinemetic(bool _bIsOn)
	{
		/*physx::PxRigidBody* pRb = dynamic_cast<physx::PxRigidBody>(m_actor);
		if (pRb)
			pRb->setRigidBodyFlag(physx::PxRigidBodyFlag::eKINEMATIC, _bIsOn);*/
	}
}