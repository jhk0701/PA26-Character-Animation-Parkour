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
	void SetCorrectAxis(ECorrectAxis _axis) { m_corrextAxis = _axis; }

private:
	Character* m_pChar{ nullptr };
	ECorrectAxis m_corrextAxis{ ECorrectAxis::XZ };

	float m_elapsedTime{ 0.0f };
	float m_properDistance{ 1.0f };
	MiniEngine::Vector3 m_startPos;
};

class BezierCorrectRootMotion : public MiniEngine::AnimNotifyState 
{
public:
	void OnStart(MiniEngine::AnimNotifyParam& _param) override;
	void OnEnd(MiniEngine::AnimNotifyParam& _param) override;
	void Activate(float _dt, MiniEngine::AnimNotifyParam& _param) override;
	
	// 캐릭터 트랜스폼 기준
	// x : 캐릭터 Right
	// y : 캐릭터 Up
	// z : 캐릭터 Forward
	void SetMidOffset(const MiniEngine::Vector3& _offset) { m_midOffset = _offset; }
	void SetEndOffset(const MiniEngine::Vector3& _offset) { m_endOffset = _offset; }

private:
	Character* m_pChar{ nullptr };

	float m_elapsedTime{ 0.0f };

	MiniEngine::Vector3 m_midOffset{ 0.0f, 0.0f, 0.0f };
	MiniEngine::Vector3 m_endOffset{ 0.0f, 0.0f, 0.0f };

	MiniEngine::Vector3 m_startPoint{ 0.0f, 0.0f, 0.0f };
	MiniEngine::Vector3 m_midPoint{ 0.0f, 0.0f, 0.0f };
	MiniEngine::Vector3 m_endPoint{ 0.0f, 0.0f, 0.0f };
};
