#pragma once
#include "Perception/Node/DetectObstacleNode.h"

using namespace MiniEngine;

class DetectObstacleUsingInputNode : public DetectObstacleSphereNode 
{
public:
	EPerceptionResult InvokeTask(TravelContext& _context, TravelResult& _result) override;
};