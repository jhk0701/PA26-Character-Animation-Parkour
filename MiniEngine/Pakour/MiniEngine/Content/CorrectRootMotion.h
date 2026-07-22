#pragma once
#include "Animation/AnimNotify.h"

class Character;

enum class ECorrectAxis : uint8_t
{
	XZ,
	XY,
	YZ,
	None
};

class CorrectRootMotion : public MiniEngine::AnimNotifyState
{
public:
	void OnStart(MiniEngine::AnimNotifyParam& _param) override;
	void Activate(float _dt, MiniEngine::AnimNotifyParam& _param) override;
	void SetProperDistance(float _distance) { m_properDistance = _distance; }
	void SetLerpWeight(float _weight) { m_lerpWeight = std::clamp(_weight, 0.0f, 1.0f); }
	void SetCorrectAxis(ECorrectAxis _axis) { m_corrextAxis = _axis; }
	void SetDeltaIntensity(float _intensity) { m_deltaIntensity = _intensity; }

private:
	Character* m_pChar{ nullptr };
	ECorrectAxis m_corrextAxis{ ECorrectAxis::XZ };

	float m_properDistance{ 1.0f };
	float m_lerpWeight{ 0.5f };
	float m_deltaIntensity{ 1.0f };
};

class BezierCorrectRootMotion : public MiniEngine::AnimNotifyState 
{
public:
	void OnStart(MiniEngine::AnimNotifyParam& _param) override;
	void Activate(float _dt, MiniEngine::AnimNotifyParam& _param) override;
	void SetBezierY(const float _y) { m_bezierY = _y; }

private:
	Character* m_pChar{ nullptr };

	float m_elapsedTime{ 0.0f };

	float m_bezierY{ 0.0f };
	MiniEngine::Vector3 m_startPoint{ 0.0f, 0.0f, 0.0f };
	MiniEngine::Vector3 m_midPoint{ 0.0f, 0.0f, 0.0f };
	MiniEngine::Vector3 m_endPoint{ 0.0f, 0.0f, 0.0f };
};