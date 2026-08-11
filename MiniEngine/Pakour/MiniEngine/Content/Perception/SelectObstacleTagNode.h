#pragma once
#include "Perception/PerceptionComponent.h"

using namespace MiniEngine;

class SelectObstacleTagNode : public SelectorNode
{
public:
	uint8_t InvokeCondition(TravelContext& _context) override;
};

