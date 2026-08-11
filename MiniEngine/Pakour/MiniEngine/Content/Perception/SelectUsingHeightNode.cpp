#include "pch.h"
#include "Content/Perception/SelectUsingHeightNode.h"
#include "Content/Perception/PerceptionNodeUtil.h"

#include "Perception/Interface/IObstacle.h"
#include "Content/Character.h"
#include "Content/Data/CharacterConfigData.h"

#include "Content/ContentConfig.h"

uint8_t SelectUsingHeightNode::InvokeCondition(TravelContext& _context)
{
	std::shared_ptr<Character> pChar = PerceptionNodeUtil::ToChar(_context.m_owner);
	const Vector3& FWD = pChar->GetRoot()->localTransform.Forward();
	const uint8_t BAND = PerceptionNodeUtil::MeasureObstacleHeight(_context, FWD);
	const PerceptionConfig& CONFIG = pChar->GetPerceptionConfig();

	if (BAND == 0)
		return (uint8_t)0; // 꼭대기가 발보다 낮다 -> CCT stepOffset 이 처리할 턱

	return BAND >= CONFIG.maxHeightStep ?  
		2 : // 최대 높이까지 측정완료
		1;  // 그 중간 사이 높이
}
