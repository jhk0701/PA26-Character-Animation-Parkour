#include "pch.h"
#include "Perception/Node/ProcessObstacleNode.h"
#include "Scene/Actor.h"

namespace MiniEngine
{
	EPerceptionResult ProcessBeamNode::InvokeTask(TravelContext& _context, TravelResult& _result)
	{
		_context.m_ledge = _context.m_pFirstObstacle->GetNearestLedgeHeight(_context.m_firstObstacleHitPos);
		_context.m_bDetectLedge = true;

		return EPerceptionResult::Succeess;
	}

	EPerceptionResult ProcessProtrudeNode::InvokeTask(TravelContext& _context, TravelResult& _result)
	{
		_context.m_firstObstacleHitPos = _context.m_pFirstObstacle->GetTransform().position;

		_context.m_ledge = _context.m_pFirstObstacle->GetNearestLedgeHeight(_context.m_firstObstacleHitPos);
		_context.m_bDetectLedge = true;

		return EPerceptionResult::Succeess;
	}

	EPerceptionResult ProcessPoleNode::InvokeTask(TravelContext& _context, TravelResult& _result)
	{
		const float LEDGE = _context.m_pFirstObstacle->GetNearestLedgeHeight(_context.m_firstObstacleHitPos);
		const float CHAR_Y = _context.m_owner->GetRoot()->localTransform.position.y;
		const float DIFF = LEDGE - CHAR_Y;

		_context.m_firstObstacleHitPos = _context.m_pFirstObstacle->GetTransform().position;

		if (DIFF >= m_heightLimit)
			_context.m_ledge = CHAR_Y + m_heightLimit;
		else
		{
			_context.m_ledge = LEDGE;
			_context.m_bDetectLedge = true;
		}

		// MG_LOG_INFO("[PoleExtractDataNode::InvokeTask] : Pole Pos x = {:.2f}, z = {:.2f}", POLE_POS.x, POLE_POS.z);

		return EPerceptionResult::Succeess;
	}

}