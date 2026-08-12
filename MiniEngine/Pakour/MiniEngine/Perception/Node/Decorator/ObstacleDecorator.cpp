#include "pch.h"
#include "Perception/Node/Decorator/ObstacleDecorator.h"
#include "Perception/Config/ObstacleConfig.h"

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

	bool CompareLedgeDecorator::Evaluate(const TravelContext& _context) const
	{
		if (!_context.m_bDetectLedge)
			return false;

		return Compare(_context.m_ledge);
	}
}
