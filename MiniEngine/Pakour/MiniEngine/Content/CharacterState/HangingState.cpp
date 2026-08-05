#include "pch.h"
#include "Content/CharacterState/HangingState.h"
#include "Content/Character.h"
#include "Platform/Input.h"

#include "Scene/Scene.h"
#include "Physics/PhysicsWorld.h"

#include "Scene/IObstacle.h"
#include "Content/Data/CharacterPerceptionConfig.h"
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