#include "pch.h"
#include "Content/CharacterState/InAirState.h"
#include "Content/Character.h"
#include "Content/ContentConfig.h"
#include "Scene/Scene.h"
#include "Physics/PhysicsWorld.h"
#include "Scene/IObstacle.h"

#include "Core/Log.h"

using namespace MiniEngine;
using namespace MiniEngine::Physics;
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

	// 착지 시점에서 장애물 한번 확인
	uint8_t tag(0U);
	if (CheckDown(tag) && tag != (uint8_t)ETagEnvDetail::Default) 
	{

		return;
	}

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

bool InAirState::CheckDown(uint8_t& _outTag)
{
	std::shared_ptr<Character> pChar = GetMachine()->GetCharacter();
	std::shared_ptr<Physics::PhysicsWorld> pPhysics = pChar->GetScene()->GetPhysics().lock();

	const Transform& TF = pChar->GetRoot()->localTransform;

	RaycastParam param;
	param.m_dir = Vector3(0.0f, -1.0f, 0.0f);
	param.m_maxDistance = 0.1f;
	param.m_origin = TF.position;

	RaycastResult result;
	bool bIsHit = pPhysics->Raycast(param, result, ToMask(Layer::Obstacle));
	if (!bIsHit)
		return false;

	Actor* pActor = reinterpret_cast<Actor*>(result.GetActor());
	if (!pActor)
		return false;

	IObstacle* pObstacle = dynamic_cast<IObstacle*>(pActor);
	if (!pObstacle)
		return false;

	pObstacle->TryGetTag(TAG_ENV_DETAIL, _outTag);

	return bIsHit;
}
