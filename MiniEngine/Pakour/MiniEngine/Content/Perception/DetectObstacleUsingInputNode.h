#pragma once
#include "Perception/Node/DetectObstacleNode.h"

using namespace MiniEngine;

class DetectObstacleUsingInputNode : public DetectObstacleCapsuleNode 
{
public:
	EPerceptionResult InvokeTask(TravelContext& _context, TravelResult& _result) override;
};