#pragma once
#include "Animation/AnimNotify.h"

class JumpTiming : public MiniEngine::AnimNotify
{
public:
	void Activate(MiniEngine::AnimNotifyParam& _param) override;
};
