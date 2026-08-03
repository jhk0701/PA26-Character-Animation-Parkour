#include "pch.h"
#include "Content/Perception/MeasureDepthNode.h"

#include "Content/Character.h"
#include "Content/Perception/PerceptionNodeUtil.h"
#include "Core/Log.h"

EPerceptionResult MeasureDepthNode::InvokeTask(TravelContext& _context, TravelResult& _result)
{
	const Vector3& FWD = PerceptionNodeUtil::ToChar(_context.m_owner)->GetRoot()->localTransform.Forward();
	PerceptionNodeUtil::MeasureObstacleDepth(_context, FWD);
	// MG_LOG_INFO("[QueryTree] ledge : {}, depth : {}", _context.m_ledge, _context.m_depth);

	return EPerceptionResult::Succeess;
}
