#pragma once
#include "Animation/AnimNotify.h"

class Character;

class CorrectRootMotion : public MiniEngine::AnimNotifyState
{
public:
	enum ECorrectAxis
	{
		XZ,
		XY,
		YZ,
		None
	};

	void OnStart(MiniEngine::AnimNotifyParam& _param) override;
	void Activate(float _dt, MiniEngine::AnimNotifyParam& _param) override;

	 void SetProperDistance(float _distance) { m_properDistance = _distance; }
	 void SetLerpWeight(float _weight) { m_lerpWeight = std::clamp(_weight, 0.0f, 1.0f); }
	 void SetCorrectAxis(ECorrectAxis _axis) { m_corrextAxis = _axis; }
	 void SetDeltaIntensity(float _intensity) { m_deltaIntensity = _intensity; }

private:
	Character* m_pChar{ nullptr };
	ECorrectAxis m_corrextAxis{ XZ };

	float m_properDistance{ 1.0f };
	float m_lerpWeight{ 0.5f };
	float m_deltaIntensity{ 1.0f };
};