#include "pch.h"
#include "Scene/PerceptionComponent.h"
#include "Scene/Scene.h"
#include "Physics/PhysicsWorld.h"

using namespace MiniEngine::Physics;

namespace MiniEngine 
{
	void PerceptionComponent::OnAttach()
	{
		Component::OnAttach();
		m_physics = owner.lock()->GetScene()->GetPhysics();
	}

	TravelResult PerceptionComponent::Travel()
	{
		if (m_physics.expired() || IsInitialized() == false)
			return TravelResult();

		TravelContext context;
		context.m_owner = owner.lock();
		context.m_physics = owner.lock()->GetScene()->GetPhysics().lock();
		context.m_units = 0;

		return m_queryTree->Execute(context);
	}
	
}