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

	pChar->ReserveIKDetectWall(); // ik 작업 예약

	Refresh();
}

void HangingState::OnEnd()
{
	std::shared_ptr<Character> pChar = GetMachine()->GetCharacter();
	pChar->SetUseGravity(true); // 매달림 해제

	pChar->ClearIKReserve(); // ik 작업 해제
}

void HangingState::Tick(float _dt)
{
	ProcessMovement(_dt);
}

void HangingState::LateTick(float _dt)
{
}

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
}

// 위에서 ledge 를 찾음 -> 올라간다.
// 상태 전환은 여기서 하지 않는다 — 클립에 붙은 TransitionState 노티파이가 Landing 으로 보낸다.
// 여기서 같이 전환하면 모션이 시작되기도 전에 쿼리 루트가 Hanging 에서 빠져나간다
void HangingState::OnPerceiveUp(const Character::PerceptedObstacleInfo& _info)
{
	std::shared_ptr<Character> pChar = GetMachine()->GetCharacter();

	MG_LOG_INFO("[Hanging] Ledge found above -> mantle (ledge : {})", _info.m_obstacleLedge);

	if (std::shared_ptr<ActionClip> pAct = pChar->GetActions((uint8_t)ETagAct::Wall_HangToMantle))
		pChar->PlayActionClip(pAct, 0.2f, (uint8_t)EActionPriority::Override);
}

// 아래에서 딛을 지면을 찾음 -> 내려온다. 전환은 위와 같은 이유로 클립이 담당
void HangingState::OnPerceiveDown(const Character::PerceptedObstacleInfo& _info)
{
	std::shared_ptr<Character> pChar = GetMachine()->GetCharacter();

	MG_LOG_INFO("[Hanging] Ground found below -> drop (ledge : {})", _info.m_obstacleLedge);

	if (std::shared_ptr<ActionClip> pAct = pChar->GetActions((uint8_t)ETagAct::Wall_HangToIdle))
		pChar->PlayActionClip(pAct, 0.2f, (uint8_t)EActionPriority::Override);
}

// 좌우에서 새 장애물을 찾음 -> 종류에 맞는 상태로 전환
void HangingState::OnPerceiveSide(const Character::PerceptedObstacleInfo& _info)
{
	std::shared_ptr<Character> pChar = GetMachine()->GetCharacter();

	uint8_t detailTag = 0;
	if (_info.m_pObstacle->TryGetTag(TAG_ENV_DETAIL, detailTag) == false)
		detailTag = (uint8_t)ETagEnvDetail::Default;

	Character::EState nextState = Character::EState::Hanging;
	switch ((ETagEnvDetail)detailTag)
	{
	case ETagEnvDetail::Beam:
		nextState = Character::EState::BeamHanging; // 매달린 채로 봉으로 옮겨감
		break;
	case ETagEnvDetail::Protrude:
		nextState = Character::EState::ProtrudeHanging;
		break;
	default:
		// 같은 벽 계열 -> 상태 유지. 같은 상태로 Transition 하면 Refresh 가 불려
		// AlignToNormal 로 새 벽면에 다시 정렬된다
		break;
	}

	MG_LOG_INFO("[Hanging] Side obstacle found -> state {}", (uint8_t)nextState);
	pChar->TransitionStateMachine((uint8_t)nextState);
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

	if (eAct != ETagAct::End && CheckEnableToMove(eAct) == false)
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
	const float CHECK_DIST = pChar->GetPerceptionConfig().onHangingSearchDist;

	RaycastParam param;
	param.m_maxDistance = CHECK_DIST;
	param.m_dir = TF.Forward();
	param.m_origin = TF.position + Vector3(0.0f, pChar->GetCapsuleHalfHeight(), 0.0f);

	switch (_tag)
	{
	case Content::Config::ETagAct::Wall_HangingMoveUp:
		param.m_origin += TF.Up() * CHECK_DIST;
		break;
	case Content::Config::ETagAct::Wall_HangingMoveDown:
		param.m_origin += TF.Up() * -CHECK_DIST;
		break;
	case Content::Config::ETagAct::Wall_HangingMoveLeft:
		param.m_origin += TF.Right() * -CHECK_DIST;
		break;
	case Content::Config::ETagAct::Wall_HangingMoveRight:
		param.m_origin += TF.Right() * CHECK_DIST;
		break;
	}

	RaycastResult result;
	return physics->Raycast(param, result, ToMask(Layer::Obstacle));
}