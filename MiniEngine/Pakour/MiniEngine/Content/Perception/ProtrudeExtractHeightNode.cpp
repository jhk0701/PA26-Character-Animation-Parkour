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
	_context.m_firstObstacleHitPos = Vector3::Lerp(_context.m_firstObstacleHitPos, _context.m_pFirstObstacle->GetTransform().position, 0.5f);

	// MG_LOG_INFO("[ProtrudeExtractHeightNode::InvokeTask] Check Protrude Height");

    return EPerceptionResult::Succeess;
}
