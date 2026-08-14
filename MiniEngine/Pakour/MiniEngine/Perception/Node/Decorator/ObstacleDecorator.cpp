#include "pch.h"
#include "Perception/Node/Decorator/ObstacleDecorator.h"
#include "Perception/Config/ObstacleConfig.h"

#include "Scene/Actor.h"

namespace MiniEngine 
{
	bool ObstacleDetectedDecorator::Evaluate(const TravelContext& _context) const
	{
		return _context.m_pFirstObstacle != nullptr;
	}

	bool CompareObstacleTypeDecorator::Evaluate(const TravelContext& _context) const
	{
		uint8_t tag = 0U;
		if (_context.m_pFirstObstacle->TryGetTag(TAG_ENV_DETAIL, tag) == false)
			return false;

		return tag == GetValue();
	}

	bool CompareHeightDecorator::Evaluate(const TravelContext& _context) const
	{
		const float FOOT_Y = _context.m_owner->GetRoot()->localTransform.position.y;
		return Compare(_context.m_ledge - FOOT_Y);
	}

	bool CompareDepthDecorator::Evaluate(const TravelContext& _context) const
	{
		return Compare(_context.m_depth);
	}
}
