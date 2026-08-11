#pragma once
#include "Scene/PerceptionComponent.h"

using namespace MiniEngine;

class ProtrudeExtractHeightNode : public TaskNode
{
public:
	EPerceptionResult InvokeTask(TravelContext& _context, TravelResult& _result) override;
};

class PoleExtractDataNode : public TaskNode 
{
public:
	EPerceptionResult InvokeTask(TravelContext& _context, TravelResult& _result) override;

	void SetHeightLimit(const float& _limit) { m_heightLimit = _limit; }
private:
	float m_heightLimit;
};