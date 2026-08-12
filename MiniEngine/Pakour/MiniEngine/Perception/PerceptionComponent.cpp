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

	bool PerceptionNode::Evaluate(const TravelContext& _context) const
	{
		if (m_conditions.size() == 0)
			return true;

		// 단일 노드 내, 다중 조건은 기본 && 로 처리할 것
		// 한 노드에 조건 두 개 넣은건 그럴 둘다 통과해야한다고 의도한 것으로 간주함
		for (const std::shared_ptr<PerceptionDecorator>& pCond : m_conditions)
		{
			if (pCond->Evaluate(_context) == false)
				return false;
		}

		return true;
	}

	EPerceptionResult TaskNode::Execute(TravelContext& _context, TravelResult& _result)
	{
		return InvokeTask(_context, _result);
	}

	EPerceptionResult SequenceNode::Execute(TravelContext& _context, TravelResult& _result)
	{
		const std::vector<std::shared_ptr<PerceptionNode>>& children = GetChildren();
		for (const std::shared_ptr<PerceptionNode>& child : children)
		{
			// 1. 자식에 달린 조건 확인 -> false라면 중단
			// 2. 자식 노드들 실행 -> 하나라도 fail이라면 중단
			if (child->Evaluate(_context) == false  ||
				child->Execute(_context, _result) == EPerceptionResult::Fail)
				return EPerceptionResult::Fail;
		}

		return EPerceptionResult::Succeess;
	}

	EPerceptionResult SelectorNode::Execute(TravelContext& _context, TravelResult& _result)
	{
		const std::vector<std::shared_ptr<PerceptionNode>>& children = GetChildren();
		for (const std::shared_ptr<PerceptionNode>& child : children)
		{
			// 1. 자식에 달린 조건 확인 -> false라면 다음 자식 확인
			if (child->Evaluate(_context) == false)
				continue;

			// 2. 자식 실행 결과 하나라도 Success 라면 종료
			if (child->Execute(_context, _result) == EPerceptionResult::Succeess)
				return  EPerceptionResult::Succeess;
		}

		return EPerceptionResult::Fail;
	}

	// 조건 노드가 만들어졌으니 제거할 것
	EPerceptionResult SwitchNode::Execute(TravelContext& _context, TravelResult& _result)
	{
		uint8_t r = InvokeCondition(_context);
		assert(r < GetChildrenCnt());

		return GetChildren()[r]->Execute(_context, _result);
	}
	EPerceptionResult BinaryConditionNode::Execute(TravelContext& _context, TravelResult& _result)
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