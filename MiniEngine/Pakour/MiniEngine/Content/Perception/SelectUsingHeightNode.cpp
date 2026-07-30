#include "pch.h"
#include "Content/Perception/SelectUsingHeightNode.h"
#include "Content/Perception/PerceptionNodeUtil.h"

#include "Scene/IObstacle.h"
#include "Content/Character.h"

#include "Content/ContentConfig.h"

uint8_t SelectUsingHeightNode::InvokeCondition(TravelContext& _context)
{
	std::shared_ptr<Character> pChar = PerceptionNodeUtil::ToChar(_context.m_owner);
	const Vector3& FWD = pChar->GetRoot()->localTransform.Forward();
	const uint8_t BAND = PerceptionNodeUtil::MeasureObstacleHeight(_context, FWD);

	if (BAND == 0)
		return (uint8_t)0; // 꼭대기가 발보다 낮다 -> CCT stepOffset 이 처리할 턱

	if (BAND >= pChar->GetPerceptionConfig().maxHeightStep)
		return (uint8_t)2; // 3.0m 이상 -> 벽 판정

	return (uint8_t)1; // 넘거나 오를 수 있는 높이 -> 깊이 측정으로
}
