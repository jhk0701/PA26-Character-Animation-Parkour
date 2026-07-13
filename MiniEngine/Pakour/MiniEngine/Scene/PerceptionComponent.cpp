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
	}
}