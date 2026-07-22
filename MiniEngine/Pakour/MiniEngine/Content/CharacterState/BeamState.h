#pragma once
#include "Content/CharacterStateMachine.h"

class BeamState : public CharacterState 
{
public:
	virtual void OnStart() override;
	virtual void OnEnd() override;
	virtual void Tick(float _dt) override;
	virtual void CheckState() override;
	virtual void ProcessPerceptionResult(const Character::PerceptedObstacleInfo& _info) override;
};

// Beam 지형에 올라탄 상태
// 움직임을 snap해줄 것
class BeamStandState : public BeamState
{
public:
	void OnStart() override;
	void OnEnd() override;
	void Tick(float _dt) override;
	void CheckState() override;
	void ProcessPerceptionResult(const Character::PerceptedObstacleInfo& _info) override;
};

// Beam 지형에 올라탄 상태
// 움직임을 snap해줄 것
class BeamHangingState : public BeamState
{
public:
};