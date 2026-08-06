#include "pch.h"
#include "Content/Processor/ObstacleCondition.h"
#include "Scene/PerceptionComponent.h"
#include "Scene/IObstacle.h"

#include "Content/ContentConfig.h"
#include "Content/Character.h"
#include "Content/Data/CharacterPerceptionConfig.h"

#include "Core/Log.h"

using namespace Content::Config;
using namespace ProcessorConditionUtil;

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

bool ObstacleHeightCondition::Evaluate(const TravelResult& _result, const ProcessContext& _context) const
{
	// 측정한 모서리 높이가 캐릭터의 발 위치 + 기준 높이보다 높다
	std::shared_ptr<Character> pChar = ToChar(_context.pOwner);

	return (GetValue() + pChar->GetRoot()->localTransform.position.y) <= _result.m_obstacleLedge;
}

bool ObstacleDepthCondition::Evaluate(const TravelResult& _result, const ProcessContext& _context) const
{
	// MG_LOG_INFO("[ObstacleDepthCondition] Compare {:.2f}, {:.2f} :: {}", GetValue(), _result.m_obstacleDepth, GetValue() <= _result.m_obstacleDepth ? "true" : "false");

	// 측정한 깊이가 기준 깊이보다 깊음
	return GetValue() <= _result.m_obstacleDepth;
}

bool ObstacleIsFrontCondition::Evaluate(const TravelResult& _result, const ProcessContext& _context) const
{
	std::shared_ptr<Character> pChar = ToChar(_context.pOwner);
	const Transform& TF = pChar->GetRoot()->localTransform;

	Vector3 point = _result.m_firstObstacleHitPos;
	point.y = 0.0f;
	Vector3 charPos = TF.position;
	charPos.y = 0.0f;

	Vector3 dir = point - charPos;
	dir.Normalize();
	Vector3 charFwd = TF.Forward();

	return dir.Dot(charFwd) > 0.0f;
}

bool ObstacleIsChangedCondition::Evaluate(const TravelResult& _result, const ProcessContext& _context) const
{
	std::shared_ptr<Character> pChar = ToChar(_context.pOwner);
	return pChar->GetCurObstacleInfo().m_bIsNewObstacle;
}

bool DetectLedgeCondition::Evaluate(const TravelResult& _result, const ProcessContext& _context) const
{
	return _result.m_bDetectLedge;
}

bool ObstacleHitDistanceCondition::Evaluate(const TravelResult& _result, const ProcessContext& _context) const
{
	// MG_LOG_INFO("[ObstacleHitDistanceCondition] GetValue : {:.2f}, HitDist : {:.2f}", GetValue(), _result.m_obstacleDistance);
	return GetValue() < _result.m_obstacleDistance;
}


