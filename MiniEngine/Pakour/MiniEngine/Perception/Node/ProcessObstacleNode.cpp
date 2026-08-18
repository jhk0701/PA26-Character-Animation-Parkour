#include "pch.h"
#include "Perception/Node/ProcessObstacleNode.h"
#include "Scene/Actor.h"

#include "Core/Log.h"

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

		const Vector3 OBS_POS = _context.m_pFirstObstacle->GetTransform().position;
		_context.m_firstObstacleHitPos.x = OBS_POS.x;
		_context.m_firstObstacleHitPos.z = OBS_POS.z;

		if (DIFF >= m_heightLimit)
			_context.m_ledge = CHAR_Y + m_heightLimit;
		else
		{
			_context.m_ledge = LEDGE;
			_context.m_bDetectLedge = true;
		}

		// MG_LOG_INFO("[ProcessPoleNode::InvokeTask] hit pos : {:.2f}, {:.2f}, {:.2f}", _context.m_firstObstacleHitPos.x, _context.m_ledge, _context.m_firstObstacleHitPos.z);

		return EPerceptionResult::Succeess;
	}

	EPerceptionResult ProcessDirectNode::InvokeTask(TravelContext& _context, TravelResult& _result)
	{
		_context.m_ledge = _context.m_pFirstObstacle->GetTransform().position.y;
		_context.m_bDetectLedge = true;

		// MG_LOG_INFO("[ProcessDirectNode::InvokeTask] hit pos : {:.2f}, {:.2f}, {:.2f}", _context.m_firstObstacleHitPos.x, _context.m_ledge, _context.m_firstObstacleHitPos.z);

		return EPerceptionResult::Succeess;
	}

}