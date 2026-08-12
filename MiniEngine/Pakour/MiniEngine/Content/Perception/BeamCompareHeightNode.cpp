#include "pch.h"
#include "Content/Perception/BeamCompareHeightNode.h"

EPerceptionResult BeamCompareHeightNode::InvokeTask(TravelContext& _context, TravelResult& _result)
{
	_context.m_ledge = _context.m_pFirstObstacle->GetNearestLedgeHeight(_context.m_firstObstacleHitPos);

	return EPerceptionResult::Succeess;
}
