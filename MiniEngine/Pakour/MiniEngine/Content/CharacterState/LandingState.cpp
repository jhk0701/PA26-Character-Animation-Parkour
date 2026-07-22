#include "pch.h"
#include "Content/CharacterState/LandingState.h"
#include "Content/Character.h"

void LandingState::OnStart() 
{
	std::shared_ptr<Character> pChar = GetMachine()->GetCharacter();
	pChar->TranstionBaseTrack(static_cast<uint8_t>(pChar->GetState()), 0.25f);
}

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
	CharacterState::CheckState();

	std::shared_ptr<Character> pChar = GetMachine()->GetCharacter();

	if (pChar->IsFalling() == false)
		return;

	// 떨어지는 중
	pChar->SetState(Character::EState::InAir);
	const uint8_t STATE = (uint8_t)pChar->GetState();
	
	// 각각의 State에서 OnStart 시, 실행해 줄 것
	// pChar->TranstionBaseTrack(STATE, 0.25f);
	GetMachine()->Transition(STATE);
}