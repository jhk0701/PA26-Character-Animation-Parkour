#include "pch.h"
#include "Perception/Node/ReturnNode.h"

EPerceptionResult ReturnResultNode::InvokeTask(TravelContext& _context, TravelResult& _result)
{
	_result = std::move(_context.intermediate);

	return EPerceptionResult::Succeess;
}

EPerceptionResult ReturnEmptyNode::InvokeTask(TravelContext& _context, TravelResult& _result)
{
	return EPerceptionResult::Fail;
}
