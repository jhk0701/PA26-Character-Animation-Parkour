#pragma once
#include "Animation/AnimNotify.h"

class UseGravity : public MiniEngine::AnimNotify
{
public:
	void Activate(MiniEngine::AnimNotifyParam& _param) override;
	void SetUseGravity(bool _bUse) { m_bUseGravity = _bUse; }

private:
	bool m_bUseGravity{ true };
};