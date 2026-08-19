#pragma once
#include "Animation/AnimNotify.h"

class UseGravityNotifyState : public MiniEngine::AnimNotifyState
{
protected:
	void OnStart(MiniEngine::AnimNotifyParam& _param) override;
	void Activate(float _dt, MiniEngine::AnimNotifyParam& _param) override {};
	void OnEnd(MiniEngine::AnimNotifyParam& _param) override;
};