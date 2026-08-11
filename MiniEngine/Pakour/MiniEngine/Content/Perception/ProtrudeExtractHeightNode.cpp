#include "pch.h"
#include "ProtrudeExtractHeightNode.h"

#include "Content/Character.h"
#include "Content/Perception/PerceptionNodeUtil.h"
#include "Core/Log.h"

using namespace MiniEngine::Physics;

EPerceptionResult ProtrudeExtractHeightNode::InvokeTask(TravelContext& _context, TravelResult& _result)
{
	_context.m_ledge = _context.m_pFirstObstacle->GetNearestLedgeHeight(_context.m_firstObstacleHitPos);
	_context.m_bDetectLedge = true;
	_context.m_firstObstacleHitPos = Vector3::Lerp(_context.m_firstObstacleHitPos, _context.m_pFirstObstacle->GetTransform().position, 0.9f);

	// MG_LOG_INFO("[ProtrudeExtractHeightNode::InvokeTask] Check Protrude Height");

    return EPerceptionResult::Succeess;
}

EPerceptionResult PoleExtractDataNode::InvokeTask(TravelContext& _context, TravelResult& _result)
{
	std::shared_ptr<Character> pChar = PerceptionNodeUtil::ToChar(_context.m_owner);
	if (!pChar)
		return EPerceptionResult::Fail;

	const float LEDGE = _context.m_pFirstObstacle->GetNearestLedgeHeight(_context.m_firstObstacleHitPos);
	const float CHAR_Y = pChar->GetRoot()->localTransform.position.y;
	const float DIFF = LEDGE - CHAR_Y;

	if (DIFF >= m_heightLimit)
		_context.m_ledge = CHAR_Y + m_heightLimit;
	else
	{
		_context.m_ledge = LEDGE;
		_context.m_bDetectLedge = true;
	}

	const Vector3 POLE_POS = _context.m_pFirstObstacle->GetTransform().position;
	_context.m_firstObstacleHitPos.x = POLE_POS.x;
	_context.m_firstObstacleHitPos.z = POLE_POS.z;

	// MG_LOG_INFO("[PoleExtractDataNode::InvokeTask] : Pole Pos x = {:.2f}, z = {:.2f}", POLE_POS.x, POLE_POS.z);

	return EPerceptionResult::Succeess;
}

