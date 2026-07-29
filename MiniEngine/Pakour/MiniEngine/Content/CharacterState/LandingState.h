#pragma once
#include "Content/CharacterStateMachine.h"

class LandingState : public CharacterState
{
public:
	void OnStart() override;
	void OnEnd() override;
	void Tick(float _dt) override;
	void LateTick(float _dt) override;
	void CheckState() override;
	void ProcessPerceptionResult(const Character::PerceptedObstacleInfo& _info) override;

private:
	Vector3 DetectLand();
};
