#pragma once
#include "Scene/PerceptionComponent.h"

using namespace MiniEngine;

class TransitionCharacterFSMNode : public TaskNode
{
public:
	EPerceptionResult InvokeTask(TravelContext& _context, TravelResult& _result) override;
	void SetTargetState(uint8_t _state) { m_state = _state; }

private:
	uint8_t m_state;
};

