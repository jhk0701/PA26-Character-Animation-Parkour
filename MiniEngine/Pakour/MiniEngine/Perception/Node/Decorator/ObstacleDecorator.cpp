#include "pch.h"
#include "Perception/Node/Decorator/ObstacleDecorator.h"
#include "Perception/Config/ObstacleConfig.h"

#include "Scene/Actor.h"

namespace MiniEngine 
{
	bool ObstacleDetectedDecorator::Evaluate(const TravelContext& _context) const
	{
		return _context.intermediate.pObstacle != nullptr;
	}

	bool CompareObstacleTypeDecorator::Evaluate(const TravelContext& _context) const
	{
		uint8_t tag = 0U;
		if (_context.intermediate.pObstacle->TryGetTag(TAG_ENV_DETAIL, tag) == false)
			return false;

		return tag == GetValue();
	}

	bool CompareHeightDecorator::Evaluate(const TravelContext& _context) const
	{
		const float FOOT_Y = _context.owner->GetRoot()->localTransform.position.y;
		return Compare(_context.intermediate.obstacleLedge - FOOT_Y);
	}

	bool CompareDepthDecorator::Evaluate(const TravelContext& _context) const
	{
		return Compare(_context.intermediate.obstacleDepth);
	}
}
