#pragma once
#include "Scene/PerceptionComponent.h"

using namespace MiniEngine;

class CheckObstacleNode : public ConditionNode
{
public:
	bool InvokeCondition(TravelContext& _context) override;
	
	void SetHeightMultiplier(float _h) { m_heightMultipier = _h; }
	void SetStartOffset(const Vector3& _offset) { m_startOffset = _offset; }

private:
	float m_heightMultipier{ 1.0f };
	Vector3 m_startOffset{ 0.0f, 0.0f, 0.0f };
};
