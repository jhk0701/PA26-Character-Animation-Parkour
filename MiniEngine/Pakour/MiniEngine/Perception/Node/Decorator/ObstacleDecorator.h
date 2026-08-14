#pragma once
#include "Perception/PerceptionComponent.h"
#include "Perception/Node/Decorator/InheritDecorator.h"

namespace MiniEngine 
{
	class ObstacleDetectedDecorator : public PerceptionDecorator
	{
	protected:
		bool Evaluate(const TravelContext& _context) const override;
	};

	class CompareObstacleTypeDecorator : public CompareWithValueDecorator<uint8_t>
	{
	protected:
		bool Evaluate(const TravelContext& _context) const override;
	};

	class CompareHeightDecorator : public CompareWithValueDecorator<float> 
	{
	protected:
		bool Evaluate(const TravelContext& _context) const override;
	};

	class CompareDepthDecorator : public CompareWithValueDecorator<float>
	{
	protected:
		bool Evaluate(const TravelContext& _context) const override;
	};
}