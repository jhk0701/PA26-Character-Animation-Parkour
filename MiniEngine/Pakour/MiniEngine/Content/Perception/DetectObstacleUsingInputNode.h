#pragma once
#include "Perception/Node/DetectObstacleNode.h"

using namespace MiniEngine;

class DetectObstacleUsingInputNode : public DetectObstacleCapsuleNode 
{
public:
	EPerceptionResult InvokeTask(TravelContext& _context, PerceptResult& _result) override;
};