#pragma once
#include "Perception/PerceptionComponent.h"

using namespace MiniEngine;

class SelectUsingHeightNode : public SwitchNode
{
public:
	uint8_t InvokeCondition(TravelContext& _context) override;
};

