#include "pch.h"
#include "Content/CharacterState/InAirState.h"

#include "Scene/Scene.h"
#include "Perception/Config/ObstacleConfig.h"
#include "Perception/Interface/IObstacle.h"

#include "Content/Character.h"
#include "Content/ContentConfig.h"

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
	if (pChar->IsGrounded() == false &&
		pChar->IsFalling())
		return;

	CheckDown(); // 아래쪽 장애물 확인 절차
}

void InAirState::ProcessContiniousMovement(float _dt)
{
	std::shared_ptr<Character> pChar = GetMachine()->GetCharacter();

	Vector3 decayMovement = Vector3::Lerp(pChar->GetPrevForce(), Vector3(0.0f), 0.75f * _dt);
	pChar->SetForce(decayMovement);
	pChar->AddMovementInput(decayMovement * _dt);
}

void InAirState::CheckDown()
{
	std::shared_ptr<Character> pChar = GetMachine()->GetCharacter();
	std::shared_ptr<Physics::PhysicsWorld> pPhysics = pChar->GetScene()->GetPhysics().lock();

	const Transform& TF = pChar->GetRoot()->localTransform;

	SpherecastParam param;
	param.m_dir = Vector3(0.0f, -1.0f, 0.0f);
	param.m_maxDistance = 0.1f;
	param.m_startPos = TF.position;
	param.m_radius = pChar->GetCapsuleRadius();

	RaycastResult result;
	bool bIsHit = pPhysics->SphereCast(param, result, Layer::Obstacle | Layer::Ground);
	if (!bIsHit)
		return;
	
	Actor* pActor = reinterpret_cast<Actor*>(result.GetActor());
	IObstacle* pObstacle = dynamic_cast<IObstacle*>(pActor);
	
	uint8_t tag = 0U;
	if (pObstacle == nullptr || 
		pObstacle->TryGetTag(TAG_ENV_DETAIL, tag) == false)
	{
		DefaultFallback();
		return;
	}

	WriteCurrentObstacleInfo(pObstacle, result);

	switch ((ETagEnvDetail)tag)
	{
	case ETagEnvDetail::Beam:
	{
		if (std::shared_ptr<ActionClip> pClip = pChar->GetActions((uint8_t)ETagAct::BeamStand))
			pChar->PlayActionClip(pClip, 0.2f);
	}
		break;
	case ETagEnvDetail::Protrude: __fallthrough;
	case ETagEnvDetail::Pole: __fallthrough;
	default:
		DefaultFallback();
		return;
	}
}

void InAirState::DefaultFallback()
{
	std::shared_ptr<Character> pChar = GetMachine()->GetCharacter();

	// 공중 -> 착지 모션
	if (std::shared_ptr<ActionClip> pClip = pChar->GetActions((uint8_t)ETagAct::FallingToLand))
		pChar->PlayActionClip(pClip, 0.2f);

	pChar->SetState(Character::EState::Landing);
	const uint8_t STATE = (uint8_t)pChar->GetState();

	// 각각의 State에서 OnStart 시, 실행해 줄 것
	// pChar->TranstionBaseTrack(STATE, 0.25f);
	GetMachine()->Transition(STATE);
}

void InAirState::WriteCurrentObstacleInfo(IObstacle* _pObstacle, const MiniEngine::Physics::RaycastResult& _hitResult)
{
	std::shared_ptr<Character> pChar = GetMachine()->GetCharacter();
	PerceptedObstacleInfo& info = pChar->GetCurObstacleInfo();

	info.m_pObstacle = _pObstacle;
	info.m_bIsNewObstacle = true;
	info.m_obstacleDistance = _hitResult.m_distance;
	info.m_obstacleDepth = 0.0f;
	info.m_obstacleHeight = 0.0f;
	info.m_obstacleHitNrm = _hitResult.m_nrm;
	info.m_obstacleHitPos = _hitResult.m_pos;
	info.m_obstacleLedge = info.m_obstacleHitPos.y;
}
