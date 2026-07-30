#pragma once
#include "Scene/PerceptionComponent.h"

using namespace MiniEngine;

class SelectCharacterStateNode : public SelectorNode
{
public:
	uint8_t InvokeCondition(TravelContext& _context) override;
};

