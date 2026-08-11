#include "pch.h"
#include "Content/Perception/SelectObstacleTagNode.h"

#include "Perception/Interface/IObstacle.h"
#include "Perception/Config/ObstacleConfig.h"
#include "Core/Log.h"

uint8_t SelectObstacleTagNode::InvokeCondition(TravelContext& _context)
{
	uint8_t detailTag = (uint8_t)MiniEngine::ETagEnvDetail::Default;
	_context.m_pFirstObstacle->TryGetTag(MiniEngine::TAG_ENV_DETAIL, detailTag);

	return detailTag;
}
