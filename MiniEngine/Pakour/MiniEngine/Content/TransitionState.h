#pragma once
#include "Animation/AnimNotify.h"

class TransitionState : public MiniEngine::AnimNotify
{
public:
	void Activate(MiniEngine::AnimNotifyParam& _param) override;
	void SetState(uint8_t _targetState) { m_targetState = _targetState; }

private:
	uint8_t m_targetState{ 0U };
};
