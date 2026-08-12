#pragma once
#include "Perception/PerceptionComponent.h"

using namespace MiniEngine;

class SelectUsingInputDirNode : public SwitchNode
{
public:
	uint8_t InvokeCondition(TravelContext& _context) override;
};

class SelectUsingInputVerticalNode : public SwitchNode
{
public:
	uint8_t InvokeCondition(TravelContext& _context) override;
};