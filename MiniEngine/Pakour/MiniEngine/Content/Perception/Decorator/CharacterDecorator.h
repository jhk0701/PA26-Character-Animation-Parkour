#pragma once
#include "Perception/PerceptionComponent.h"
#include "Perception/Node/Decorator/InheritDecorator.h"

using namespace MiniEngine;

class CharacterStateDecorator : public CompareWithValueDecorator<uint8_t>
{
protected:
	bool Evaluate(const TravelContext& _context) const override;
};

class InputVerticalDecorator : public CompareWithValueDecorator<float> 
{
protected:
	bool Evaluate(const TravelContext& _context) const override;
};

class InputHorizontalDecorator : public CompareWithValueDecorator<float> 
{
protected:
	bool Evaluate(const TravelContext& _context) const override;
};