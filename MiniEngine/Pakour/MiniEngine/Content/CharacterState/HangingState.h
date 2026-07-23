#pragma once
#include "Content/CharacterState/CameraFixedState.h"
#include "Content/ContentConfig.h"

class HangingState : public CameraFixedState
{
public:
	void OnStart() override;
	void OnEnd() override;
	void Tick(float _dt) override;
	void ProcessPerceptionResult(const Character::PerceptedObstacleInfo& _info) override;

private:
	void ProcessMovement(float _dt);
	bool CheckEnableToMove(Content::Config::ETagAct _tag);
};