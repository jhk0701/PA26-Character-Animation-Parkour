#include "pch.h"
#include "Content/Processor/ObstacleCondition.h"
#include "Scene/PerceptionComponent.h"
#include "Scene/IObstacle.h"

#include "Content/ContentConfig.h"
#include "Content/Character.h"
#include "Content/Data/CharacterPerceptionConfig.h"

#include "Content/Processor/ProcessorConditionUtil.h"

using namespace Content::Config;
using namespace ProcessorConditionUtil;

bool ObstacleTypeCondition::Evaluate(const TravelResult& _result, const ProcessContext& _context) const
{
	if (!_result.m_pFirstObstacle)
		return false;

	uint8_t type = 0;
	if (_result.m_pFirstObstacle->TryGetTag(TAG_ENV_DETAIL, type) == false)
		return false;

	return type == m_target;
}

// 자주 쓰는 타입은 사용하기 편하게 미리 선언
IsDefault::IsDefault() { SetTarget((uint8_t)ETagEnvDetail::Default); }
IsBeam::IsBeam() { SetTarget((uint8_t)ETagEnvDetail::Beam); }

bool ObstacleHeightCondition::Evaluate(const TravelResult& _result, const ProcessContext& _context) const
{
	std::shared_ptr<Character> pChar = ToChar(_context.pOwner);
	return m_height <= _result.m_obstacleLedge - pChar->GetRoot()->localTransform.position.y;
}
