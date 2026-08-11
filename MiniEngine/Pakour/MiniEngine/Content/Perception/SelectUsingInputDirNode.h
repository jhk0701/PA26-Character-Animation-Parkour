#pragma once
#include "Perception/PerceptionComponent.h"

using namespace MiniEngine;

class SelectUsingInputDirNode : public SelectorNode
{
public:
	uint8_t InvokeCondition(TravelContext& _context) override;
};

class SelectUsingInputVerticalNode : public SelectorNode
{
public:
	uint8_t InvokeCondition(TravelContext& _context) override;
};