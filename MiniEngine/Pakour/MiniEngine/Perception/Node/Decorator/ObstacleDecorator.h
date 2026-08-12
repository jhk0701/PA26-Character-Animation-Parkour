#pragma once
#include "Perception/PerceptionComponent.h"
#include "Perception/Node/Decorator/InheritDecorator.h"

namespace MiniEngine 
{
	class ObstacleDetectedDecorator : public PerceptionDecorator
	{
	public:
		bool Evaluate(const TravelContext& _context) const override;
	};

	class CompareObstacleTypeDecorator : public CompareWithValueDecorator<uint8_t>
	{
	public:
		bool Evaluate(const TravelContext& _context) const override;
	};

	class CompareHeightDecorator : public CompareWithValueDecorator<float> 
	{
	public:
		bool Evaluate(const TravelContext& _context) const override;
	};
}