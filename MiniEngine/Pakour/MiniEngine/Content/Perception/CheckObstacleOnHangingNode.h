#pragma once
#include "Scene/PerceptionComponent.h"

using namespace MiniEngine;

class CheckOnHangingMoveUpNode : public ConditionNode
{
public:
	bool InvokeCondition(TravelContext& _context) override;
};

class CheckOnHangingUpwardLedgeNode: public ConditionNode
{
public:
	bool InvokeCondition(TravelContext& _context) override;
	void SetStartOffset(const Vector3& _offset) { m_startOffset = _offset; }
private:
	Vector3 m_startOffset{ 0.0f, 0.0f, 0.0f };
};

class CheckOnHangingMoveDownNode : public ConditionNode
{
public:
	bool InvokeCondition(TravelContext& _context) override;
};

class CheckOnHangingMoveSideNode : public ConditionNode
{
public:
	bool InvokeCondition(TravelContext& _context) override;
	void SetStartOffset(const Vector3& _offset) { m_startOffset = _offset; }
private:
	Vector3 m_startOffset{ 0.0f, 0.0f, 0.0f };
};
;