#include "pch.h"
#include "Content/CharacterState/LandingState.h"
#include "Core/Log.h"

void LandingState::OnStart() 
{
	std::shared_ptr<Character> pChar = GetMachine()->GetCharacter();
	pChar->TranstionBaseTrack(static_cast<uint8_t>(pChar->GetState()), 0.25f);
	
	// 바닥 IK 작업 예약
	pChar->ReserveIKDetectGround();
}

void LandingState::OnEnd()
{
	std::shared_ptr<Character> pChar = GetMachine()->GetCharacter();
	pChar->InputLerp() = Vector2(0.0f);
	
	// 예약한 IK 작업 정리
	pChar->ClearIKReserve();
}

void LandingState::Tick(float _dt)
{
	ProcessMovement(_dt);
	SyncControllerRotate();

	CheckState();
}

void LandingState::LateTick(float _dt) {}

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

void LandingState::ProcessPerceptionResult(const Character::PerceptedObstacleInfo& _info)
{
	DefaultProcessPerceptionResult(_info);
}
