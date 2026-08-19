#include "pch.h"
#include "Perception//PerceptionComponent.h"
#include "Scene/Scene.h"
#include "Physics/PhysicsWorld.h"

using namespace MiniEngine::Physics;

namespace MiniEngine 
{
#pragma region Perception Nodes

	bool PerceptionDecorator::Process(const TravelContext& _context)
	{
		bool bResult = Evaluate(_context);

		if (m_bIsInvert)
			bResult = !bResult;

		return bResult;
	}

	bool PerceptionNode::Evaluate(const TravelContext& _context) const
	{
		if (m_conditions.size() == 0)
			return true;

		// 단일 노드 내, 다중 조건은 기본 && 로 처리할 것
		// 한 노드에 조건 두 개 넣은건 그럴 둘다 통과해야한다고 의도한 것으로 간주함
		for (const std::shared_ptr<PerceptionDecorator>& pCond : m_conditions)
		{
			if (pCond->Process(_context) == false)
				return false;
		}

		return true;
	}

	EPerceptionResult TaskNode::Execute(TravelContext& _context, PerceptResult& _result)
	{
		return InvokeTask(_context, _result);
	}

	EPerceptionResult SequenceNode::Execute(TravelContext& _context, PerceptResult& _result)
	{
		const std::vector<std::shared_ptr<PerceptionNode>>& children = GetChildren();
		for (const std::shared_ptr<PerceptionNode>& child : children)
		{
			// 1. 자식에 달린 조건 확인 -> false라면 중단
			// 2. 자식 노드들 실행 -> 하나라도 fail이라면 중단
			// Sequence 인데 Deco를 달았다면, 조건 불일치 시 종료 요청으로 간주
			if (child->Evaluate(_context) == false  ||
				child->Execute(_context, _result) == EPerceptionResult::Fail)
				return EPerceptionResult::Fail;
		}

		return EPerceptionResult::Succeess;
	}

	EPerceptionResult SelectorNode::Execute(TravelContext& _context, PerceptResult& _result)
	{
		const std::vector<std::shared_ptr<PerceptionNode>>& children = GetChildren();
		for (const std::shared_ptr<PerceptionNode>& child : children)
		{
			// 1. 자식에 달린 조건 확인 -> false라면 다음 자식 확인
			if (child->Evaluate(_context) == false)
				continue;

			// 2. 자식 실행 결과 하나라도 Success 라면 종료
			// 자식 실행 결과가 fail이라면 다음 자식 실행 시도
			if (child->Execute(_context, _result) == EPerceptionResult::Succeess)
				return  EPerceptionResult::Succeess;
		}

		// 모든 자식이 fail인 경우
		return EPerceptionResult::Fail;
	}

#pragma endregion

	EPerceptionResult PerceptionComponent::Travel(uint8_t _idx, const Vector3& _dir)
	{
		if (IsInitialized() == false || m_queries.size() <= _idx)
			return EPerceptionResult::Fail;
		
		m_result.Reset();

		TravelContext context;
		context.owner = owner.lock();
		context.physics = context.owner->GetScene()->GetPhysics().lock();
		context.direction = _dir;
		context.intermediate.Reset();

		return m_queries[_idx]->Execute(context, m_result);
	}

	bool PerceptionComponent::TryGetPerceptedInfo(
		PerceptedObstacleInfo& _out, 
		const IObstacle* _prevObstacle /*= nullptr*/)
	{
		if (!m_result.pObstacle)
			return false;

		_out.bIsNewObstacle = _prevObstacle ? 
			_prevObstacle != m_result.pObstacle : 
			true;
		
		_out.obstacleHeight = m_result.obstacleLedge - owner.lock()->GetRoot()->localTransform.position.y;
		_out.perceptResult = std::move(m_result);

		return true;
	}

}