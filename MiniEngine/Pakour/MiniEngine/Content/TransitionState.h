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

class EnableHangingState : public MiniEngine::AnimNotify
{
public:
	void Activate(MiniEngine::AnimNotifyParam& _param) override;
	void SetEnable(bool _bEnable) { m_bEnable = _bEnable; }

private:
	bool m_bEnable{ true };
};
