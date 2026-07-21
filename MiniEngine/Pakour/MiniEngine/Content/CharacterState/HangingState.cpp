#include "pch.h"
#include "Content/CharacterState/HangingState.h"
#include "Content/Character.h"
#include "Content/ContentConfig.h"
#include "Platform/Input.h"

using namespace Content::Config;

void HangingState::OnStart()
{
	std::shared_ptr<Character> pChar = GetMachine()->GetCharacter();
	pChar->SetUseGravity(false); // 매달린 중에는 중력 적용 해제

	pChar->TranstionBaseTrack(static_cast<uint8_t>(pChar->GetState()), 0.25f);
}
void HangingState::OnEnd()
{
	std::shared_ptr<Character> pChar = GetMachine()->GetCharacter();
	pChar->SetUseGravity(true); // 매달림 해제

	pChar->TranstionBaseTrack(static_cast<uint8_t>(pChar->GetState()), 0.25f);
}

void HangingState::Tick(float _dt)
{
	ProcessMovement(_dt);
	CameraRotate(_dt);

	CheckState();
}

void HangingState::CheckState()
{
	// 일차적으로 풀 수 있는 경우
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

	if (std::shared_ptr<ActionClip> pAct = pChar->GetActions((uint8_t)eAct))
		pChar->PlayActionClip(pAct, 0.1f);
}
