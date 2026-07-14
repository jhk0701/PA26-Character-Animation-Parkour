#include "pch.h"
#include "Scene/PerceptionComponent.h"
#include "Scene/Scene.h"
#include "Physics/PhysicsWorld.h"

using namespace MiniEngine::Physics;

namespace MiniEngine 
{
	void ConditionNode::SetCondition(std::function<bool(TravelContext&)>&& _cond, 
		std::shared_ptr<QueryNodeBase> _nodeOnTrue, 
		std::shared_ptr<QueryNodeBase> _nodeOnFalse)
	{
		m_condition = _cond;

		m_child.resize(2);
		m_child[0] = _nodeOnTrue;
		m_child[1] = _nodeOnFalse;
	}

	TravelResult ConditionNode::Execute(TravelContext& _context)
	{
		if (!m_condition)
			return TravelResult();

		if (m_condition(_context))
			return m_child[0]->Execute(_context);
		else
			return m_child[1]->Execute(_context);
	};

	TravelResult LeafNode::Execute(TravelContext& _context)
	{
		if (!m_task)
			return TravelResult();

		return m_task(_context);
	}


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