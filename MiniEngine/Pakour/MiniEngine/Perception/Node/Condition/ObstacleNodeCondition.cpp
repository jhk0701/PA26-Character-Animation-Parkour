#include "pch.h"
#include "Perception/Node/Condition/ObstacleNodeCondition.h"

namespace MiniEngine 
{
	bool ObstacleDetectedDecorator::Evaluate(const TravelContext& _context) const
	{
		return _context.m_pFirstObstacle != nullptr;
	}


}
