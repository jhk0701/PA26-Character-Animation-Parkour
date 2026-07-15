#pragma once
#include "Animation/AnimNotify.h"

class Character;

class CorrectRootMotion : public MiniEngine::AnimNotifyState
{
public:
	void OnStart(MiniEngine::AnimNotifyParam& _param) override;
	void Activate(float _dt, MiniEngine::AnimNotifyParam& _param) override;

	 void SetProperDistance(float _distance) { m_properDistance = _distance; }

private:
	Character* m_pChar{ nullptr };
	float m_properDistance{ 1.0f };
};