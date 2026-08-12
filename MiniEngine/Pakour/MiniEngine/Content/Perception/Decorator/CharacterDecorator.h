#pragma once
#include "Perception/PerceptionComponent.h"
#include "Perception/Node/Decorator/InheritDecorator.h"

using namespace MiniEngine;

class CharacterStateDecorator : CompareWithValueDecorator<uint8_t>
{
public:
	bool Evaluate(const TravelContext& _context) const override;
};