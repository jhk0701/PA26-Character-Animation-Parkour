#pragma once
#include "Content/CharacterState/RotateFixState.h"

class ProtrudeState : public RotateFixState
{
public:
	void OnStart() override;
	void OnEnd() override;
	void Refresh() override;

	void Tick(float _dt) override;
	void ProcessPerceptionResult(const Character::PerceptedObstacleInfo& _result) override;

private:
	void AlignToNormal();
};