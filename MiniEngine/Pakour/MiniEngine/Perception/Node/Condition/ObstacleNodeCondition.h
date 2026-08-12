#pragma once
#include "Perception/PerceptionComponent.h"

namespace MiniEngine 
{
	class ObstacleDetectedDecorator : public PerceptionDecorator
	{
	public:
		bool Evaluate(const TravelContext& _context) const override;
	};
}