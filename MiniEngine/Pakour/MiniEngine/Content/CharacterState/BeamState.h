#pragma once
#include "Content/CharacterStateMachine.h"

// Beam 지형에 올라탄 상태
// 움직임을 snap해줄 것
class BeamState : public CharacterState
{
public:
	void OnStart() override;
	void OnEnd() override;
	void Tick(float _dt) override;
	void CheckState() override;
};