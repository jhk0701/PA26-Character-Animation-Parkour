#pragma once
#include "Perception/PerceptionComponent.h"

using namespace MiniEngine;

class SelectCharacterStateNode : public SwitchNode
{
public:
	uint8_t InvokeCondition(TravelContext& _context) override;
};