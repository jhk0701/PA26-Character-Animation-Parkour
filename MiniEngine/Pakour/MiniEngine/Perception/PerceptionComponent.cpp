#include "pch.h"
#include "Perception//PerceptionComponent.h"
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
		m_bDetectLedge = false;
	}

#pragma region Perception Nodes

	EPerceptionResult TaskNode::Execute(TravelContext& _context, TravelResult& _result)
	{
		return InvokeTask(_context, _result);
	}

	EPerceptionResult SequenceNode::Execute(TravelContext& _context, TravelResult& _result)
	{
		for (size_t i = 0; i < GetChildrenCnt(); ++i)
		{
			if (GetChildren()[i]->Execute(_context, _result) == EPerceptionResult::Fail)
				return  EPerceptionResult::Fail;
		}

		return EPerceptionResult::Succeess;
	}

	EPerceptionResult SelectorNode::Execute(TravelContext& _context, TravelResult& _result)
	{
		for (size_t i = 0; i < GetChildrenCnt(); ++i)
		{
			if (GetChildren()[i]->Execute(_context, _result) == EPerceptionResult::Succeess)
				return  EPerceptionResult::Succeess;
		}

		return EPerceptionResult::Fail;
	}

	EPerceptionResult SwitchNode::Execute(TravelContext& _context, TravelResult& _result)
	{
		uint8_t r = InvokeCondition(_context);
		assert(r < GetChildrenCnt());

		return GetChildren()[r]->Execute(_context, _result);
	}

	EPerceptionResult ConditionNode::Execute(TravelContext& _context, TravelResult& _result)
	{
		if (InvokeCondition(_context))
			return GetChildren()[0]->Execute(_context, _result);
		else
			return GetChildren()[1]->Execute(_context, _result);
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