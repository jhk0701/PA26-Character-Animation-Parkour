#include "pch.h"
#include "Content/CharacterState/LandingState.h"
#include "Content/Character.h"

void LandingState::OnStart() {}
void LandingState::OnEnd()
{
	GetMachine()->GetCharacter()->InputLerp() = Vector2(0.0f);
}

void LandingState::Tick(float _dt)
{
	DefaultMovement(_dt);
	DefaultCameraRotate(_dt);

	CheckState();
}

void LandingState::CheckState()
{
	std::shared_ptr<Character> pChar = GetMachine()->GetCharacter();

	if (pChar->IsFalling() == false)
		return;

	// 떨어지는 중
	pChar->SetState(Character::EState::InAir);
	const uint8_t STATE = (uint8_t)pChar->GetState();

	pChar->TranstionBaseTrack(STATE);
	GetMachine()->Transition(STATE);
}