#pragma once
#include "Scene/ProcessorComponent.h"

using namespace MiniEngine;

class ObstacleTypeCondition : public ProcessCondition
{
public:
	bool Evaluate(const TravelResult& _result, const ProcessContext& _context) const override;
	void SetTarget(uint8_t _target) { m_target = _target; }

private:
	uint8_t m_target;
};

class IsDefault : public ObstacleTypeCondition 
{
public:
	IsDefault();
};

class IsBeam : public ObstacleTypeCondition
{
public:
	IsBeam();
};


class ObstacleHeightCondition : public ProcessCondition 
{
public:
	bool Evaluate(const TravelResult& _result, const ProcessContext& _context) const override;
	void SetHeight(float _height) { m_height = _height; }

private:
	float m_height;
};
