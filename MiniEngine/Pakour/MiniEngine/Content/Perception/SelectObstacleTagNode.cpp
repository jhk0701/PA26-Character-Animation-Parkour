#include "pch.h"
#include "Content/Perception/SelectObstacleTagNode.h"

#include "Content/ContentConfig.h"
#include "Scene/IObstacle.h"
#include "Core/Log.h"

uint8_t SelectObstacleTagNode::InvokeCondition(TravelContext& _context)
{
	uint8_t detailTag = (uint8_t)Content::Config::ETagEnvDetail::Default;
	_context.m_pFirstObstacle->TryGetTag(Content::Config::TAG_ENV_DETAIL, detailTag);

	switch ((Content::Config::ETagEnvDetail)detailTag)
	{
	case Content::Config::ETagEnvDetail::Default:
		MG_LOG_INFO("[SelectObstacleTagNode] :: is Default tag");
		break;
	case Content::Config::ETagEnvDetail::Beam:
		MG_LOG_INFO("[SelectObstacleTagNode] :: is Beam tag");
		break;
	case Content::Config::ETagEnvDetail::Protrude:
		MG_LOG_INFO("[SelectObstacleTagNode] :: is Protrude tag");
		break;
	}

	return detailTag;
}
