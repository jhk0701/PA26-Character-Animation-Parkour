#include "pch.h"
#include "PerceptionComponent.h"
#include "Scene/Actor.h"
#include "Scene/Scene.h"
#include "Physics/PhysicsWorld.h"
#include "Core/Log.h"

namespace MiniEngine 
{
	void PerceptionComponent::OnAttach()
	{
		Component::OnAttach();

		m_physics = owner.lock()->GetScene()->GetPhysics();
	}

	void PerceptionComponent::Tick(float _dt)
	{
		Component::Tick(_dt);
	}

	void PerceptionComponent::StartTravel()
	{
		if (m_physics.expired())
			return;

		MG_LOG_INFO("Perception Travel");

		std::shared_ptr<SceneComponent> pRoot = owner.lock()->GetRoot();

		Physics::RaycastParam rayParam;
		rayParam.m_dir = pRoot->localTransform.Forward();
		rayParam.m_maxDistance = m_maxObsDist;
		rayParam.m_origin = pRoot->localTransform.position + Vector3(0.0f, 1.0f, 0.0f);
		Physics::RaycastResult rayResult;

		m_physics.lock()->Raycast(rayParam, rayResult);
	}
}