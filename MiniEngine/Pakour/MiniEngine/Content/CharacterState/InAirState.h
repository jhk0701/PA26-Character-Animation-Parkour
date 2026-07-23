#pragma once
#include "Content/CharacterStateMachine.h"

class InAirState : public CharacterState
{
public:
	void OnStart() override;
	void OnEnd() override;
	void Tick(float _dt) override;
	void CheckState() override;
	void ProcessPerceptionResult(const Character::PerceptedObstacleInfo& _info) override;
};
