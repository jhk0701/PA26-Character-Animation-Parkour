#pragma once
#include "Perception/ProcessorComponent.h"
#include "Perception/Condition/ProcessorConditionUtil.h"

using namespace MiniEngine;

class ObstacleDetectedCondition : public CompareWithValueCondition<uint8_t>
{
protected:
	bool Evaluate(const TravelResult& _result, const ProcessContext& _context) const override;
};

class ObstacleTypeCondition : public CompareWithValueCondition<uint8_t>
{
protected:
	bool Evaluate(const TravelResult& _result, const ProcessContext& _context) const override;
};

class ObstacleHeightCondition : public CompareWithValueCondition<float>
{
protected:
	bool Evaluate(const TravelResult& _result, const ProcessContext& _context) const override;
};

class ObstacleDepthCondition : public CompareWithValueCondition<float>
{
protected:
	bool Evaluate(const TravelResult& _result, const ProcessContext& _context) const override;
};

// 장애물의 위치가 캐릭터의 위상에서 앞에 있는지 확인
class ObstacleIsFrontCondition : public ProcessCondition
{
protected:
	bool Evaluate(const TravelResult& _result, const ProcessContext& _context) const override;
};

class DetectLedgeCondition : public ProcessCondition
{
protected:
	bool Evaluate(const TravelResult& _result, const ProcessContext& _context) const override;
};

// 탐지한 hit 거리 비교
class ObstacleHitDistanceCondition : public CompareWithValueCondition<float>
{
protected:
	bool Evaluate(const TravelResult& _result, const ProcessContext& _context) const override;
};