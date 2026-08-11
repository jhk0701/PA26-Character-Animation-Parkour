#include "pch.h"
#include "Content/CharacterState/LandingState.h"
#include "Scene/Scene.h"
#include "Perception/Interface/IObstacle.h"
#include "Content/Data/CharacterConfigData.h"
#include "Core/Log.h"
#include "Core/DebugMarkers.h"

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

	// 떨어지기 전 최종으로 아래에 장애물은 없는지 확인
	if (IsOnFloor())
		return;

	// MG_LOG_INFO("[CharacterState::CheckState] 낙하 처리");

	// 떨어지는 중
	pChar->SetState(Character::EState::InAir);
	const uint8_t STATE = (uint8_t)pChar->GetState();

	// 각각의 State에서 OnStart 시, 실행해 줄 것
	// pChar->TranstionBaseTrack(STATE, 0.25f);
	GetMachine()->Transition(STATE);
}

bool LandingState::IsOnFloor()
{
	std::shared_ptr<Character> pChar = GetMachine()->GetCharacter();
	std::shared_ptr<MiniEngine::Physics::PhysicsWorld> pPhysic = pChar->GetScene()->GetPhysics().lock();
	
	MiniEngine::Physics::RaycastParam param;
	param.m_origin = pChar->GetRoot()->localTransform.position;
	param.m_dir = Vector3(0.0f, -1.0f, 0.0f);
	param.m_maxDistance = pChar->GetConfig().onLandingFallingCheckDist;

	MiniEngine::Physics::RaycastResult result;
	return pPhysic->Raycast(param, result, MiniEngine::Physics::Layer::Ground | MiniEngine::Physics::Layer::Obstacle);
}
