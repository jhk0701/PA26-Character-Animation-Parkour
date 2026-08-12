#pragma once
#include "Perception/PerceptionComponent.h"

using namespace MiniEngine;

class CheckObstacleNode : public BinaryConditionNode
{
public:
	bool InvokeCondition(TravelContext& _context) override;
	
	void SetHeightMultiplier(float _h) { m_heightMultipier = _h; }
	void SetStartOffset(const Vector3& _offset) { m_startOffset = _offset; }
	void SetDirection(const Vector3& _dir) { m_dir = _dir; }

private:
	float m_heightMultipier{ 1.0f };
	Vector3 m_startOffset{ 0.0f, 0.0f, 0.0f };
	Vector3 m_dir{ 0.0f, 0.0f, 0.0f };
};
