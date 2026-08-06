#include "pch.h"
#include "Content/CharacterState/InAirState.h"
#include "Content/Character.h"
#include "Content/ContentConfig.h"
#include "Core/Log.h"
#include "Scene/IObstacle.h"
#include "Content/Data/CharacterPerceptionConfig.h"

using namespace Content::Config;

void InAirState::OnStart() 
{
	std::shared_ptr<Character> pChar = GetMachine()->GetCharacter();
	pChar->TranstionBaseTrack(static_cast<uint8_t>(pChar->GetState()), 0.25f);
}

void InAirState::OnEnd() 
{
	std::shared_ptr<Character> pChar = GetMachine()->GetCharacter();
	pChar->SetForce(Vector3(0.0f));
}

void InAirState::Tick(float _dt)
{
	ProcessContiniousMovement(_dt);

	CheckState();
}

void InAirState::LateTick(float _dt) {}

void InAirState::CheckState()
{
	CharacterState::CheckState();

	// 공중 + 떨어지는 상황
	// 바닥 감지 필요
	std::shared_ptr<Character> pChar = GetMachine()->GetCharacter();
	if (pChar->IsGrounded() == false)
		return;

	// 공중 -> 착지 모션
	if (std::shared_ptr<ActionClip> pClip = pChar->GetActions((uint8_t)ETagAct::FallingToLand))
		pChar->PlayActionClip(pClip, 0.2f);

	pChar->SetState(Character::EState::Landing);
	const uint8_t STATE = (uint8_t)pChar->GetState();

	// 각각의 State에서 OnStart 시, 실행해 줄 것
	// pChar->TranstionBaseTrack(STATE, 0.25f);
	GetMachine()->Transition(STATE);
}

void InAirState::ProcessContiniousMovement(float _dt)
{
	std::shared_ptr<Character> pChar = GetMachine()->GetCharacter();

	Vector3 decayMovement = Vector3::Lerp(pChar->GetPrevForce(), Vector3(0.0f), 0.75f * _dt);
	pChar->SetForce(decayMovement);
	pChar->AddMovementInput(decayMovement * _dt);
}
