#include "pch.h"
#include "Content/CharacterState/HangingState.h"
#include "Content/Character.h"
#include "Platform/Input.h"

#include "Scene/Scene.h"
#include "Physics/PhysicsWorld.h"

using namespace Content::Config;
using namespace MiniEngine::Physics;

void HangingState::OnStart()
{
	std::shared_ptr<Character> pChar = GetMachine()->GetCharacter();
	pChar->SetUseGravity(false); // 매달린 중에는 중력 적용 해제
	
	pChar->TranstionBaseTrack(static_cast<uint8_t>(pChar->GetState()), 0.25f);
	
	TakeOverCameraRotate(pChar);
}

void HangingState::OnEnd()
{
	std::shared_ptr<Character> pChar = GetMachine()->GetCharacter();
	pChar->SetUseGravity(true); // 매달림 해제
	
	pChar->TranstionBaseTrack(static_cast<uint8_t>(pChar->GetState()), 0.25f);
	
	HandOverCameraRotate(pChar);
}

void HangingState::Tick(float _dt)
{
	ProcessMovement(_dt);
	CameraRotate(_dt);
}

void HangingState::ProcessPerceptionResult(const Character::PerceptedObstacleInfo& _info)
{
	DefaultProcessPerceptionResult(_info);
}

void HangingState::CameraRotate(float _dt)
{
	std::shared_ptr<Character> pChar = GetMachine()->GetCharacter();
	Input& input = InputManager::GetInstance()->GetInput();

	// 마우스 델타에 이미 델타타임이 곱해져 있음
	const Vector2 camRotSpeed = pChar->GetCamRotateSpeed() * input.GetMouseDelta();
	Vector2& camRot = pChar->CamRotate();
	const float MAX_PITCH = pChar->GetCamPitchMaxDeg();

	camRot.x += camRotSpeed.x;
	camRot.y += camRotSpeed.y;
	camRot.y = std::clamp(camRot.y, 180.0f - MAX_PITCH, 180.0f + MAX_PITCH);

	Quaternion qRot = Quaternion::CreateFromYawPitchRoll(ToRadians(camRot.x), ToRadians(camRot.y), 0.0f);
	qRot.Normalize();
	
	std::shared_ptr<SceneComponent> pCamHolderRoot = pChar->GetCamHolder().lock();
	pCamHolderRoot->localTransform.rotation = qRot;

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
		pChar->PlayActionClip(pAct, 0.1f);
}

bool HangingState::CheckEnableToMove(ETagAct _tag)
{
	// 진행방향에 레이캐스트
	// 향하려는 방향에 아무것도 없는 경우 -> 가지 못함
	std::shared_ptr<Character> pChar = GetMachine()->GetCharacter();
	std::shared_ptr<Physics::PhysicsWorld> physics = pChar->GetScene()->GetPhysics().lock();
	const Transform& TF = pChar->GetRoot()->localTransform;
	const float CHECK_DIST = pChar->GetCheckingDistance();

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

void HangingState::TakeOverCameraRotate(std::shared_ptr<Character>& _pChar)
{
	Vector2& camRot = _pChar->CamRotate();
	m_prevYaw = camRot.x;
	camRot.x = 0.0f;
}

void HangingState::HandOverCameraRotate(std::shared_ptr<Character>& _pChar)
{
	Vector2& camRot = _pChar->CamRotate();
	camRot.x += m_prevYaw;

	Quaternion qYaw = Quaternion::CreateFromAxisAngle(Vector3::Transform(Vector3(.0f, 1.0f, .0f), Quaternion(0.0f, 0.0f, 0.0f, 1.0f)), ToRadians(camRot.x));
	Quaternion qPitch = Quaternion::CreateFromAxisAngle(Vector3(1.0f, 0.0f, 0.0f), ToRadians(camRot.y));
	qYaw.Normalize();
	qPitch.Normalize();

	_pChar->GetRoot()->localTransform.rotation = qYaw;
	_pChar->GetCamHolder().lock()->localTransform.rotation = qPitch;
}
