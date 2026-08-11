#include "pch.h"
#include "Content/Perception/BeamCompareHeightNode.h"
#include "Physics/PhysicsWorld.h"

#include "Content/Character.h"
#include "Content/Perception/PerceptionNodeUtil.h"
#include "Core/Log.h"

using namespace MiniEngine::Physics;

EPerceptionResult BeamCompareHeightNode::InvokeTask(TravelContext& _context, TravelResult& _result)
{
	_context.m_ledge = _context.m_pFirstObstacle->GetNearestLedgeHeight(_context.m_firstObstacleHitPos);

	return EPerceptionResult::Succeess;
}
