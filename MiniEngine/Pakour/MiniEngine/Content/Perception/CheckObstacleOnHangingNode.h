#pragma once
#include "Scene/PerceptionComponent.h"

using namespace MiniEngine;

class CheckOnHangingMoveUpNode : public ConditionNode
{
public:
	bool InvokeCondition(TravelContext& _context) override;
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
};
;