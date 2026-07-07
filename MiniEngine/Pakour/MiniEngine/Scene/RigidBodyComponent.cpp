#include "pch.h"
#include "Scene/RigidBodyComponent.h"
#include "Scene/Actor.h"
#include "Physics/PhysicsWorld.h"

#include <physx/PxPhysicsAPI.h>

namespace MiniEngine
{
	void RigidBodyComponent::Init(Physics::PhysicsWorld& _world, EBodyType _type, const Vector3& _halfExtents, float _denity)
	{
		m_type = _type;

		Vector3 pos(0.0f);
		Quaternion rot(0.0f, 0.0f, 0.0f, 1.0f);

		if (owner.expired())
			return;

		std::shared_ptr<Actor> pOwner = owner.lock();
		std::shared_ptr<SceneComponent> pRoot = pOwner->GetRoot();

		if (!pRoot)
			return;
		
		pos = pRoot->localTransform.position;
		rot = pRoot->localTransform.rotation;

		m_actor = (_type == EBodyType::Dynamic) ?
			_world.CreateDynamicBox(pos, rot, _halfExtents, _denity) :
			_world.CreateStaticBox(pos, rot, _halfExtents);
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
}