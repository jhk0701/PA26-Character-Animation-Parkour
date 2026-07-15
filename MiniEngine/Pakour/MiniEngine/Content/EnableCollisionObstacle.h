#pragma once
#include "Animation/AnimNotify.h"

class EnableCollisionObstacle : public MiniEngine::AnimNotify
{
public:
	void Activate(MiniEngine::AnimNotifyParam& _param) override;
	void SetEnable(bool _bEnagle) { m_bEnable = _bEnagle; }

private:
	bool m_bEnable{ true };
};
