#include "pch.h"
#include "Perception/Node/ReturnNode.h"

EPerceptionResult ReturnResultNode::InvokeTask(TravelContext& _context, PerceptResult& _result)
{
	_result = std::move(_context.intermediate);

	return EPerceptionResult::Succeess;
}

EPerceptionResult ReturnEmptyNode::InvokeTask(TravelContext& _context, PerceptResult& _result)
{
	return EPerceptionResult::Fail;
}
