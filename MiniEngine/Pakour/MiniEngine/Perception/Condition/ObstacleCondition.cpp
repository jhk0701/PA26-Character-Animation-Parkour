#include "pch.h"
#include "Perception/Condition/ObstacleCondition.h"

#include "Scene/Actor.h"
#include "Perception/Interface/IPerceptionProcessor.h"
#include "Perception/Interface/IObstacle.h"
#include "Perception/PerceptionComponent.h"
#include "Perception/Config/ObstacleConfig.h"

#include "Core/Log.h"

namespace MiniEngine 
{
	bool ObstacleDetectedCondition::Evaluate(const TravelResult& _result, const ProcessContext& _context) const
	{
		return _result.m_pFirstObstacle != nullptr;
	}

	bool ObstacleTypeCondition::Evaluate(const TravelResult& _result, const ProcessContext& _context) const
	{
		if (!_result.m_pFirstObstacle)
			return false;

		uint8_t type = 0;
		if (_result.m_pFirstObstacle->TryGetTag(TAG_ENV_DETAIL, type) == false)
			return false;

		// MG_LOG_INFO("[ObstacleTypeCondition] Result : {}", type == m_targetType ? "true" : "false");

		return type == GetValue();
	}

	bool LastObstacleTypeCondition::Evaluate(const TravelResult& _result, const ProcessContext& _context) const
	{
		IObstacle* pCurObstacle = _context.pOwner->GetCurObstacle();
		if (!pCurObstacle)
			return false;

		uint8_t type = 0U;
		if (pCurObstacle->TryGetTag(TAG_ENV_DETAIL, type) == false)
			return false;

		return type == GetValue();
	}

	bool ObstacleHeightCondition::Evaluate(const TravelResult& _result, const ProcessContext& _context) const
	{
		// 측정한 모서리 높이가 캐릭터의 발 위치 + 기준 높이보다 높다
		const float DIFF = _result.m_obstacleLedge - _context.pOwner->GetTransform().position.y;
		return GetValue() <= DIFF;
		
		// return (GetValue() + _context.pOwner->GetRoot()->localTransform.position.y) <= _result.m_obstacleLedge;
	}

	bool ObstacleDepthCondition::Evaluate(const TravelResult& _result, const ProcessContext& _context) const
	{
		// MG_LOG_INFO("[ObstacleDepthCondition] Compare {:.2f}, {:.2f} :: {}", GetValue(), _result.m_obstacleDepth, GetValue() <= _result.m_obstacleDepth ? "true" : "false");

		// 측정한 깊이가 기준 깊이보다 깊음
		return GetValue() <= _result.m_obstacleDepth;
	}

	bool ObstacleIsFrontCondition::Evaluate(const TravelResult& _result, const ProcessContext& _context) const
	{
		const Transform& TF = _context.pOwner->GetTransform();

		Vector3 point = _result.m_firstObstacleHitPos;
		point.y = 0.0f;
		Vector3 charPos = TF.position;
		charPos.y = 0.0f;

		Vector3 dir = point - charPos;
		dir.Normalize();
		Vector3 charFwd = TF.Forward();

		return dir.Dot(charFwd) > 0.0f;
	}

	bool DetectLedgeCondition::Evaluate(const TravelResult& _result, const ProcessContext& _context) const
	{
		// MG_LOG_INFO("[DetectLedgeCondition] Check : {}", _result.m_bDetectLedge);
		return _result.m_bDetectLedge;
	}

	bool ObstacleHitDistanceCondition::Evaluate(const TravelResult& _result, const ProcessContext& _context) const
	{
		// MG_LOG_INFO("[ObstacleHitDistanceCondition] GetValue : {:.2f}, HitDist : {:.2f}", GetValue(), _result.m_obstacleDistance);
		return GetValue() < _result.m_obstacleDistance;
	}


	bool DetectNewObstacle::Evaluate(const TravelResult& _result, const ProcessContext& _context) const
	{

		std::shared_ptr<IPerceptionProcessor> pProcessor = std::dynamic_pointer_cast<IPerceptionProcessor>(_context.pOwner);
		return pProcessor->GetCurObstacleInfo().m_bIsNewObstacle;
	}

}
