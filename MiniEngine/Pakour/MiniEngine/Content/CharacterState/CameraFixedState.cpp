#include "pch.h"
#include "Content/CharacterState/CameraFixedState.h"
#include "Content/Character.h"
#include "Platform/Input.h"

void CameraFixedState::OnStart()
{
	std::shared_ptr<Character> pChar = GetMachine()->GetCharacter();
	TakeOverCameraRotate(pChar);
}

void CameraFixedState::OnEnd()
{
	std::shared_ptr<Character> pChar = GetMachine()->GetCharacter();
	HandOverCameraRotate(pChar);
}

void CameraFixedState::CameraRotate(float _dt)
{
	std::shared_ptr<Character> pChar = GetMachine()->GetCharacter();
	Input& input = InputManager::GetInstance()->GetInput();

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

void CameraFixedState::TakeOverCameraRotate(std::shared_ptr<Character>& _pChar)
{
	Vector2& camRot = _pChar->CamRotate();
	m_prevYaw = camRot.x;
	camRot.x = 0.0f;
}

void CameraFixedState::HandOverCameraRotate(std::shared_ptr<Character>& _pChar)
{
	Vector2& camRot = _pChar->CamRotate();
	camRot.x += m_prevYaw;

	return;
	Quaternion qYaw = Quaternion::CreateFromAxisAngle(Vector3::Transform(Vector3(.0f, 1.0f, .0f), Quaternion(0.0f, 0.0f, 0.0f, 1.0f)), ToRadians(camRot.x));
	Quaternion qPitch = Quaternion::CreateFromAxisAngle(Vector3(1.0f, 0.0f, 0.0f), ToRadians(camRot.y));
	qYaw.Normalize();
	qPitch.Normalize();

	_pChar->GetRoot()->localTransform.rotation = qYaw;
	_pChar->GetCamHolder().lock()->localTransform.rotation = qPitch;
}
