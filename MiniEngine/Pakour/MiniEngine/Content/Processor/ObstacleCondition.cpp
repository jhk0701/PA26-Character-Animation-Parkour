#include "pch.h"
#include "Content/Processor/ObstacleCondition.h"
#include "Scene/PerceptionComponent.h"
#include "Scene/IObstacle.h"

#include "Content/ContentConfig.h"
#include "Content/Character.h"
#include "Content/Data/CharacterPerceptionConfig.h"

#include "Content/Processor/ProcessorConditionUtil.h"
#include "Core/Log.h"


using namespace Content::Config;
using namespace ProcessorConditionUtil;

bool ObstacleTypeCondition::Evaluate(const TravelResult& _result, const ProcessContext& _context) const
{
	if (!_result.m_pFirstObstacle)
		return false;

	uint8_t type = 0;
	if (_result.m_pFirstObstacle->TryGetTag(TAG_ENV_DETAIL, type) == false)
		return false;

	// MG_LOG_INFO("[ObstacleTypeCondition] Result : {}", type == m_targetType ? "true" : "false");

	return type == m_targetType;
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
