#pragma once
#include "Perception/PerceptionComponent.h"

using namespace MiniEngine;

class ReturnResultNode : public TaskNode
{
public:
	EPerceptionResult InvokeTask(TravelContext& _context, PerceptResult& _result) override;
};

class ReturnEmptyNode : public TaskNode 
{
public:
	EPerceptionResult InvokeTask(TravelContext& _context, PerceptResult& _result) override;
};