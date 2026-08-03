#include "pch.h"
#include "Content/Perception/SelectObstacleTagNode.h"

#include "Content/ContentConfig.h"
#include "Scene/IObstacle.h"

uint8_t SelectObstacleTagNode::InvokeCondition(TravelContext& _context)
{
	uint8_t detailTag = (uint8_t)Content::Config::ETagEnvDetail::Default;
	_context.m_pFirstObstacle->TryGetTag(Content::Config::TAG_ENV_DETAIL, detailTag);

	return detailTag;
}
