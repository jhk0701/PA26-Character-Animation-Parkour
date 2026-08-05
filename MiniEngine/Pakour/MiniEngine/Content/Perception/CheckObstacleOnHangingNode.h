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

// 입력한 방향으로 탐색 거리만큼 이동한 지점에 정면으로 레이를 쏴서
// 해당 벽면으로 갈 수 있는지 확인
class CheckOnHangingMoveToInputDirNode : public ConditionNode 
{
public:
	bool InvokeCondition(TravelContext& _context) override;
	void SetStartOffset(const Vector3& _offset) { m_startOffset = _offset; }
private:
	Vector3 m_startOffset{ 0.0f, 0.0f, 0.0f };
};