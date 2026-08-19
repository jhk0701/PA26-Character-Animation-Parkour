#pragma once
#include "Content/CharacterStateMachine.h"

namespace MiniEngine::Physics
{
	struct RaycastResult;
}

class InAirState : public CharacterState
{
public:
	void OnStart() override;
	void OnEnd() override;
	void Tick(float _dt) override;
	void LateTick(float _dt) override;
	void CheckState() override;

private:
	void ProcessContiniousMovement(float _dt);
};
