#include "pch.h"
#include "Perception/Node/ProcessObstacleNode.h"
#include "Scene/Actor.h"

#include "Core/Log.h"

namespace MiniEngine
{
	EPerceptionResult ProcessBeamNode::InvokeTask(TravelContext& _context, PerceptResult& _result)
	{
		PerceptResult& intermediate = _context.intermediate;
		intermediate.obstacleLedge = intermediate.pObstacle->GetNearestLedgeHeight(intermediate.obstacleHitPos);
		intermediate.bDetectLedge = true;

		return EPerceptionResult::Succeess;
	}

	EPerceptionResult ProcessProtrudeNode::InvokeTask(TravelContext& _context, PerceptResult& _result)
	{
		PerceptResult& intermediate = _context.intermediate;
		intermediate.obstacleHitPos = intermediate.pObstacle->GetTransform().position;

		intermediate.obstacleLedge = intermediate.pObstacle->GetNearestLedgeHeight(intermediate.obstacleHitPos);
		intermediate.bDetectLedge = true;

		return EPerceptionResult::Succeess;
	}

	EPerceptionResult ProcessPoleNode::InvokeTask(TravelContext& _context, PerceptResult& _result)
	{
		PerceptResult& intermediate = _context.intermediate;

		const float LEDGE = intermediate.pObstacle->GetNearestLedgeHeight(intermediate.obstacleHitPos);
		const float CHAR_Y = _context.owner->GetRoot()->localTransform.position.y;
		const float DIFF = LEDGE - CHAR_Y;

		const Vector3 OBS_POS = intermediate.pObstacle->GetTransform().position;
		intermediate.obstacleHitPos.x = OBS_POS.x;
		intermediate.obstacleHitPos.z = OBS_POS.z;

		if (DIFF >= m_heightLimit)
			intermediate.obstacleLedge = CHAR_Y + m_heightLimit;
		else
		{
			intermediate.obstacleLedge = LEDGE;
			intermediate.bDetectLedge = true;
		}

		// MG_LOG_INFO("[ProcessPoleNode::InvokeTask] hit pos : {:.2f}, {:.2f}, {:.2f}", _context.m_firstObstacleHitPos.x, _context.m_ledge, _context.m_firstObstacleHitPos.z);

		return EPerceptionResult::Succeess;
	}

	EPerceptionResult ProcessDirectNode::InvokeTask(TravelContext& _context, PerceptResult& _result)
	{
		PerceptResult& intermediate = _context.intermediate;

		intermediate.obstacleLedge = intermediate.pObstacle->GetTransform().position.y;
		intermediate.bDetectLedge = true;

		// MG_LOG_INFO("[ProcessDirectNode::InvokeTask] hit pos : {:.2f}, {:.2f}, {:.2f}", _context.m_firstObstacleHitPos.x, _context.m_ledge, _context.m_firstObstacleHitPos.z);

		return EPerceptionResult::Succeess;
	}

}