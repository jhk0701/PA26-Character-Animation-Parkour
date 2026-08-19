#pragma once
#include "Perception/ProcessorComponent.h"
#include "Perception/Condition/InheritCondition.h"

using namespace MiniEngine;

class CharacterStateCondition : public CompareWithValueCondition<uint8_t>
{
protected:
	bool Evaluate(const PerceptResult& _result, const ProcessContext& _context) const override;
};

class CharacterHeightCondition : public CompareWithValueCondition<float>
{
protected:
	bool Evaluate(const PerceptResult& _result, const ProcessContext& _context) const override;
};

class InputVerticalCondition : public CompareWithValueCondition<float>
{
protected:
	bool Evaluate(const PerceptResult& _result, const ProcessContext& _context) const override;
};

class InputHorizontalCondition : public CompareWithValueCondition<float>
{
protected:
	bool Evaluate(const PerceptResult& _result, const ProcessContext& _context) const override;
};
