#include "pch.h"
#include "Scene/PerceptionComponent.h"
#include "Scene/Scene.h"
#include "Physics/PhysicsWorld.h"

using namespace MiniEngine::Physics;

namespace MiniEngine 
{
	void TravelResult::Reset()
	{
		m_pFirstObstacle = nullptr;
		m_firstObstacleHitPos = Vector3(0.0f);
		m_firstObstacleHitNrm = Vector3(0.0f);
		m_obstacleDistance = 0.0f;
		m_obstacleLedge = 0.0f;
		m_obstacleDepth = 0.0f;
	}

#pragma region Perception Nodes

	void ConditionNode::SetChildren(std::vector<std::shared_ptr<PerceptionNode>>&& _children)
	{
		// true / false 두 갈래로 고정
		assert(_children.size() == 2);
		m_children = std::move(_children);
	}

	EPerceptionResult ConditionNode::Execute(TravelContext& _context, TravelResult& _result)
	{
		if (InvokeCondition(_context))
			return m_children[0]->Execute(_context, _result);
		else
			return m_children[1]->Execute(_context, _result);
	}

	void SelectorNode::SetChildren(std::vector<std::shared_ptr<PerceptionNode>>&& _children)
	{
		m_children = std::move(_children);
	}

	EPerceptionResult SelectorNode::Execute(TravelContext& _context, TravelResult& _result)
	{
		uint8_t r = InvokeCondition(_context);
		assert(r < m_children.size());

		return m_children[r]->Execute(_context, _result);
	}

	void SequenceNode::SetChildren(std::vector<std::shared_ptr<PerceptionNode>>&& _children)
	{
		m_children = std::move(_children);
	}

	EPerceptionResult SequenceNode::Execute(TravelContext& _context, TravelResult& _result)
	{
		EPerceptionResult result = EPerceptionResult::Succeess;

		for (size_t i = 0; i < m_children.size(); ++i)
		{
			EPerceptionResult r = m_children[i]->Execute(_context, _result);

			if (result < r)
				result = r;
		}

		return result;
	}

	EPerceptionResult TaskNode::Execute(TravelContext& _context, TravelResult& _result)
	{
		return InvokeTask(_context, _result);
	}

#pragma endregion

	void PerceptionComponent::OnAttach()
	{
		Component::OnAttach();
		m_physics = owner.lock()->GetScene()->GetPhysics();
	}

	EPerceptionResult PerceptionComponent::Travel()
	{
		if (m_physics.expired() || IsInitialized() == false)
		{
			m_result.Reset();
			return EPerceptionResult::Fail;
		}

		TravelContext context;
		context.m_owner = owner.lock();
		context.m_physics = context.m_owner->GetScene()->GetPhysics().lock();
		
		return m_queryTree->Execute(context, m_result);
	}

}