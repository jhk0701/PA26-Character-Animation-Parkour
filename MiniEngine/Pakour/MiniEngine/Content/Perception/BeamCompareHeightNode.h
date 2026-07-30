#pragma once
#include "Scene/PerceptionComponent.h"

using namespace MiniEngine;

class BeamCompareHeightNode : public TaskNode
{
public:
	EPerceptionResult InvokeTask(TravelContext& _context, TravelResult& _result) override;
};

