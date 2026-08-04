#pragma once
#include "Scene/ProcessorComponent.h"

using namespace MiniEngine;

class CompareRealNumberCondition : public ProcessCondition 
{
public:
	void SetValue(float _val) { m_value = _val; }
	float GetValue() const { return m_value; }

private:
	float m_value;
};

class ObstacleTypeCondition : public ProcessCondition
{
public:
	void SetType(uint8_t _type) { m_targetType = _type; }

protected:
	bool Evaluate(const TravelResult& _result, const ProcessContext& _context) const override;

private:
	uint8_t m_targetType;
};

class ObstacleHeightCondition : public CompareRealNumberCondition
{
protected:
	bool Evaluate(const TravelResult& _result, const ProcessContext& _context) const override;
};

class ObstacleDepthCondition : public CompareRealNumberCondition
{
protected:
	bool Evaluate(const TravelResult& _result, const ProcessContext& _context) const override;
};