#pragma once
#include "Animation/AnimNotify.h"

class Character;
class CorrectFixedRotation : public MiniEngine::AnimNotifyState
{
public:
	void OnStart(MiniEngine::AnimNotifyParam& _param) override;
	void Activate(float _dt, MiniEngine::AnimNotifyParam& _param) override;

	void SetRotateDeg(const MiniEngine::Vector3& _yawPitchRoll) { m_rotateDegree = _yawPitchRoll; }

private:
	Character* m_pChar{ nullptr };

	float m_elapsedTime{ 0.0f };

	MiniEngine::Vector3 m_rotateDegree;
	MiniEngine::Quaternion m_startRotation;
	MiniEngine::Quaternion m_endRotation;
};

class CorrectRotationTowardObstacle : public MiniEngine::AnimNotifyState
{
public:
	void OnStart(MiniEngine::AnimNotifyParam& _param) override;
	void Activate(float _dt, MiniEngine::AnimNotifyParam& _param) override;

private:
	Character* m_pChar{ nullptr };

	float m_elapsedTime{ 0.0f };

	MiniEngine::Quaternion m_startRotation;
	MiniEngine::Quaternion m_endRotation;
};
