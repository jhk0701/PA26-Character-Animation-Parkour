#include "pch.h"
#include "ProtrudeExtractHeightNode.h"

#include "Content/Character.h"
#include "Content/Perception/PerceptionNodeUtil.h"
#include "Core/Log.h"

using namespace MiniEngine::Physics;

EPerceptionResult ProtrudeExtractHeightNode::InvokeTask(TravelContext& _context, TravelResult& _result)
{
	std::shared_ptr<Character> pChar = PerceptionNodeUtil::ToChar(_context.m_owner);
	_context.m_ledge = _context.m_pFirstObstacle->GetNearestLedgeHeight(_context.m_firstObstacleHitPos);

    return EPerceptionResult::Succeess;
}
