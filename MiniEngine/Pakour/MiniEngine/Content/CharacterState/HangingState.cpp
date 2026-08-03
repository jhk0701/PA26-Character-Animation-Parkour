#include "pch.h"
#include "Content/CharacterState/HangingState.h"
#include "Content/Character.h"
#include "Platform/Input.h"

#include "Scene/Scene.h"
#include "Physics/PhysicsWorld.h"

#include "Core/Log.h"

using namespace Content::Config;
using namespace MiniEngine::Physics;

void HangingState::OnStart()
{
	std::shared_ptr<Character> pChar = GetMachine()->GetCharacter();
	pChar->SetUseGravity(false); // 매달린 중에는 중력 적용 해제
	pChar->TranstionBaseTrack(static_cast<uint8_t>(pChar->GetState()), 0.25f);

	Refresh();
}

void HangingState::OnEnd()
{
	std::shared_ptr<Character> pChar = GetMachine()->GetCharacter();
	pChar->SetUseGravity(true); // 매달림 해제
}

void HangingState::Tick(float _dt)
{
	ProcessMovement(_dt);
}

void HangingState::LateTick(float _dt){ }

void HangingState::ProcessPerceptionResult(const Character::PerceptedObstacleInfo& _info)
{
	if (_info.IsValid() == false)
		return;

	// 트리가 입력 방향으로 1발만 쐈으므로, 같은 입력으로 어느 프로브였는지 되짚는다
	const Vector2 INPUT_DIR = GetMachine()->GetCharacter()->GetInputDir();

	if (INPUT_DIR.y > 0)
		OnPerceiveUp(_info);
	else if (INPUT_DIR.y < 0)
		OnPerceiveDown(_info);
	else if (INPUT_DIR.x != 0)
		OnPerceiveSide(_info);
	else if (INPUT_DIR.x < 1e-4f && INPUT_DIR.y < 1e-4f)
		OnPerceiveUp(_info);
}

// 위에서 ledge 를 찾음 -> 올라간다.
void HangingState::OnPerceiveUp(const Character::PerceptedObstacleInfo& _info)
{
	std::shared_ptr<Character> pChar = GetMachine()->GetCharacter();

	uint8_t tag = 0;
	if (_info.m_pObstacle->TryGetTag(TAG_ENV_DETAIL, tag) == false || 
		tag == (uint8_t)ETagEnvDetail::Beam)
	{
		DefaultProcessPerceptionResult(_info);
		return;
	}
	else if (tag == (uint8_t)ETagEnvDetail::Protrude) 
	{
		if (std::shared_ptr<ActionClip> pAct = pChar->GetActions((uint8_t)ETagAct::Wall_ProtrudeMoveUp))
			pChar->PlayActionClip(pAct, 0.2f, (uint8_t)EActionPriority::Override);

		return;
	}

	const Transform& TF = pChar->GetRoot()->localTransform;

	Vector3 point = _info.m_obstacleHitPos;
	point.y = 0.0f;
	Vector3 charPos = TF.position;
	charPos.y = 0.0f;

	Vector3 dir = point - charPos;
	dir.Normalize();
	Vector3 charFwd = TF.Forward();
	
	const float DOT = dir.Dot(charFwd);
	uint8_t act = (uint8_t)(DOT > 0.0f ? ETagAct::Wall_HangToMantle : ETagAct::Wall_HangToMantleOnObs);
	// MG_LOG_INFO("[HangingState::OnPerceiveUp] Dot : {:.2f}, Act : {}", DOT, act == (uint8_t)ETagAct::Wall_HangToMantle ? "Hang To Mantle" : "Hang To Mantle Obs");

	if (std::shared_ptr<ActionClip> pAct = pChar->GetActions(act))
		pChar->PlayActionClip(pAct, 0.2f, (uint8_t)EActionPriority::Override);
}

// 아래에서 딛을 지면을 찾음 -> 내려온다. 전환은 위와 같은 이유로 클립이 담당
void HangingState::OnPerceiveDown(const Character::PerceptedObstacleInfo& _info)
{
	std::shared_ptr<Character> pChar = GetMachine()->GetCharacter();

	// MG_LOG_INFO("[Hanging] Ground found below -> drop (ledge : {})", _info.m_obstacleLedge);
	if (std::shared_ptr<ActionClip> pAct = pChar->GetActions((uint8_t)ETagAct::Wall_HangToIdle))
		pChar->PlayActionClip(pAct, 0.2f, (uint8_t)EActionPriority::Override);
}

// 좌우에서 새 장애물을 찾음 -> 종류에 맞는 상태로 전환
void HangingState::OnPerceiveSide(const Character::PerceptedObstacleInfo& _info)
{
	// 
	uint8_t tag = 0;
	if (_info.m_pObstacle->TryGetTag(TAG_ENV_DETAIL, tag) == false)
	{
		DefaultProcessPerceptionResult(_info);
		return;
	}

	DefaultProcessPerceptionResult(_info);
	return;
}

void HangingState::Refresh()
{
	CharacterState::Refresh();
	AlignToNormal();
}

void HangingState::AlignToNormal()
{
	std::shared_ptr<Character> pChar = GetMachine()->GetCharacter();

	Vector3 nrm = -pChar->GetCurObstacleInfo().m_obstacleHitNrm;
	nrm.y = 0.0f;
	nrm.Normalize();

	Quaternion rot;
	if (TryYawRotateToward(nrm, rot))
		pChar->GetRoot()->localTransform.rotation = rot;
}

void HangingState::ProcessMovement(float _dt)
{
	std::shared_ptr<Character> pChar = GetMachine()->GetCharacter();
	if (pChar->IsActionClipPlaying())
		return;

	const Vector2 INPUT_DIR = pChar->GetInputDir();

	// 4 방향 중 하나만 골라야 함
	ETagAct eAct = ETagAct::End;
	if (INPUT_DIR.y > 0)
		eAct = ETagAct::Wall_HangingMoveUp;
	else if (INPUT_DIR.y < 0)
		eAct = ETagAct::Wall_HangingMoveDown;
	else if (INPUT_DIR.x > 0)
		eAct = ETagAct::Wall_HangingMoveRight;
	else if (INPUT_DIR.x < 0)
		eAct = ETagAct::Wall_HangingMoveLeft;

	if (eAct != ETagAct::End && 
		CheckEnableToMove(eAct) == false)
		return;

	if (std::shared_ptr<ActionClip> pAct = pChar->GetActions((uint8_t)eAct))
		pChar->PlayActionClip(pAct, 0.2f);
}

bool HangingState::CheckEnableToMove(ETagAct _tag)
{
	// 진행방향에 레이캐스트
	// 향하려는 방향에 아무것도 없는 경우 -> 가지 못함
	std::shared_ptr<Character> pChar = GetMachine()->GetCharacter();
	std::shared_ptr<Physics::PhysicsWorld> physics = pChar->GetScene()->GetPhysics().lock();
	const Transform& TF = pChar->GetRoot()->localTransform;

	RaycastParam param;
	param.m_maxDistance = pChar->GetCapsuleRadius() * 2.0f;
	param.m_dir = TF.Forward();
	param.m_origin = TF.position + Vector3(0.0f, pChar->GetCapsuleHalfHeight(), 0.0f);

	switch (_tag)
	{
	case Content::Config::ETagAct::Wall_HangingMoveUp:
		param.m_origin += TF.Up();
		break;
	case Content::Config::ETagAct::Wall_HangingMoveDown:
		param.m_origin += -TF.Up();
		break;
	case Content::Config::ETagAct::Wall_HangingMoveLeft:
		param.m_origin += -TF.Right();
		break;
	case Content::Config::ETagAct::Wall_HangingMoveRight:
		param.m_origin += TF.Right();
		break;
	}

	RaycastResult result;
	return physics->Raycast(param, result, ToMask(Layer::Obstacle));
}