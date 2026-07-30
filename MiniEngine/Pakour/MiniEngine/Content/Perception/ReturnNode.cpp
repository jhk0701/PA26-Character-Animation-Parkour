#include "pch.h"
#include "Content/Perception/ReturnNode.h"

EPerceptionResult ReturnResultNode::InvokeTask(TravelContext& _context, TravelResult& _result)
{
	_result.m_pFirstObstacle = _context.m_pFirstObstacle;
	_result.m_firstObstacleHitPos = _context.m_firstObstacleHitPos;
	_result.m_firstObstacleHitNrm = _context.m_firstObstacleHitNrm;
	_result.m_obstacleDistance = _context.m_distance;
	_result.m_obstacleDepth = _context.m_depth;
	_result.m_obstacleLedge = _context.m_ledge;

	return EPerceptionResult::Succeess;
}

EPerceptionResult ReturnEmptyNode::InvokeTask(TravelContext& _context, TravelResult& _result)
{
	return EPerceptionResult::Fail;
}
