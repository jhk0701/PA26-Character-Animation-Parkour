#include "pch.h"
#include "Content/CharacterState/BeamHangingState.h"

void BeamHangingState::OnStart()
{
}

void BeamHangingState::OnEnd()
{
}

void BeamHangingState::Tick(float _dt)
{
	CheckState();
}

void BeamHangingState::CheckState()
{
	CharacterState::CheckState();
}

void BeamHangingState::ProcessPerceptionResult(const Character::PerceptedObstacleInfo& _info)
{
	DefaultProcessPerceptionResult(_info);
}
