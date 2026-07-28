#include "pch.h"
#include "Scene/PerceptionComponent.h"
#include "Scene/Scene.h"
#include "Physics/PhysicsWorld.h"

using namespace MiniEngine::Physics;

namespace MiniEngine 
{
	void TravelResult::Reset()
	{
		m_bIsEmpty = true;
		m_pFirstObstacle = nullptr;
		m_firstObstacleHitPos = Vector3(0.0f);
		m_firstObstacleHitNrm = Vector3(0.0f);
		m_obstacleDistance = 0.0f;
		m_obstacleLedge = 0.0f;
		m_obstacleDepth = 0.0f;
	}

	void ConditionNode::SetCondition(std::function<bool(TravelContext&)>&& _cond, 
		std::shared_ptr<QueryNodeBase> _nodeOnTrue, 
		std::shared_ptr<QueryNodeBase> _nodeOnFalse)
	{
		m_condition = _cond;

		m_children.resize(2);
		m_children[0] = _nodeOnTrue;
		m_children[1] = _nodeOnFalse;
	}

	TravelResult ConditionNode::Execute(TravelContext& _context)
	{
		if (!m_condition)
			return TravelResult();

		if (m_condition(_context))
			return m_children[0]->Execute(_context);
		else
			return m_children[1]->Execute(_context);
	};

	void SelectorNode::SetCondition(
		std::function<uint8_t(TravelContext&)>&& _cond, 
		std::vector<std::shared_ptr<QueryNodeBase>>&& _results
	)
	{
		m_condition = _cond;
		m_children = _results;
	}

	TravelResult SelectorNode::Execute(TravelContext& _context)
	{
		if (!m_condition)
			return TravelResult();

		uint8_t r = m_condition(_context);
		assert(r < m_children.size());

		return m_children[r]->Execute(_context);
	}

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

	void PerceptionComponent::Travel()
	{
		if (m_physics.expired() || IsInitialized() == false)
		{
			m_result.Reset();
			return;
		}

		TravelContext context;
		context.m_owner = owner.lock();
		context.m_physics = context.m_owner->GetScene()->GetPhysics().lock();

		m_result = m_queryTree->Execute(context);
		return;
	}

}