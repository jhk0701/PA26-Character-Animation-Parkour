#include "pch.h"
#include "Content/Perception/MeasureNode.h"

#include "Content/Character.h"
#include "Perception/Node/PerceptionNodeUtil.h"
#include "Core/Log.h"


EPerceptionResult MeasureDepthNode::InvokeTask(TravelContext& _context, TravelResult& _result)
{
	const Vector3& FWD = PerceptionNodeUtil::ToChar(_context.m_owner)->GetRoot()->localTransform.Forward();
	PerceptionNodeUtil::MeasureObstacleDepth(_context, FWD);
	// MG_LOG_INFO("[QueryTree] ledge : {}, depth : {}", _context.m_ledge, _context.m_depth);

	return EPerceptionResult::Succeess;
}

EPerceptionResult MeasureHeightNode::InvokeTask(TravelContext& _context, TravelResult& _result)
{
	std::shared_ptr<Character> pChar = PerceptionNodeUtil::ToChar(_context.m_owner);
	const Vector3& FWD = pChar->GetRoot()->localTransform.Forward();
	
	PerceptionNodeUtil::MeasureObstacleHeight(_context, FWD);

	return EPerceptionResult::Succeess;
}

EPerceptionResult MeasureDepth_SideNode::InvokeTask(TravelContext& _context, TravelResult& _result)
{
	std::shared_ptr<Character> pChar = PerceptionNodeUtil::ToChar(_context.m_owner);
	const Transform& TF = pChar->GetRoot()->localTransform;

	Vector3 dir = pChar->GetInputDir().x > 0 ? TF.Right() : -TF.Right();
	PerceptionNodeUtil::MeasureObstacleDepth(_context, dir);

	return EPerceptionResult::Succeess;
}

EPerceptionResult MeasureHeight_SideNode::InvokeTask(TravelContext& _context, TravelResult& _result)
{
	std::shared_ptr<Character> pChar = PerceptionNodeUtil::ToChar(_context.m_owner);
	const Transform& TF = pChar->GetRoot()->localTransform;
	
	Vector3 dir = pChar->GetInputDir().x > 0 ? TF.Right() : -TF.Right();
	PerceptionNodeUtil::MeasureObstacleHeight(_context, dir);

	return EPerceptionResult::Succeess;
}
