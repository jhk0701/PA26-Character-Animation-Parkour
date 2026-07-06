#pragma once
#include "Scene/Actor.h"

using namespace MiniEngine;

class Character : public MiniEngine::Actor
{
public:
	Character();
	virtual ~Character();

	virtual void BeginPlay() override;
	virtual void Tick(float _dt) override;

	void SetMoveSpeed(float _newSpeed) { m_moveSpeed = _newSpeed; }
	void SetInputDir(const Vector2& _dir) { m_inputDir = _dir; }
	Vector2 GetInputDir() const { return m_inputDir; }

private:
	// 이동 기능 -> 시간 남으면 CharacterMovementComponent로 리팩터링
	Vector2 m_inputDir;
	float m_moveSpeed{ 100.0f };
};

