#include "pch.h"
#include "Content/CharacterState/InAirState.h"
#include "Content/Character.h"
#include "Content/ContentConfig.h"

void InAirState::OnStart() {}
void InAirState::OnEnd() {}
void InAirState::Tick(float _dt)
{
	// 떨어지는 중엔 movement 막기
	DefaultCameraRotate(_dt);

	CheckState();
}


void InAirState::CheckState()
{
	// 공중 + 떨어지는 상황
	// 바닥 감지 필요
	std::shared_ptr<Character> pChar = GetMachine()->GetCharacter();
	if (pChar->IsGrounded() == false)
		return;

	// 공중 -> 착지 모션
	if (std::shared_ptr<ActionClip> pClip = pChar->GetActions((uint8_t)Content::Config::ETagAct::FallingToLand))
		pChar->PlayActionClip(pClip, 0.2f);

	pChar->SetState(Character::EState::Landing);
	const uint8_t STATE = (uint8_t)pChar->GetState();

	pChar->TranstionBaseTrack(STATE, 0.25f);
	GetMachine()->Transition(STATE);
}
