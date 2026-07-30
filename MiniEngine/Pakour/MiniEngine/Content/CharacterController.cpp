#include "pch.h"
#include "Platform/Input.h"
#include "Scene/CameraComponent.h"
#include "Content/CharacterController.h"
#include "Content/Character.h"
#include "Core/Log.h"

using namespace MiniEngine;

void CharacterController::Construct()
{
	Controller::Construct();

	std::shared_ptr<SceneComponent> pCamHolder = AddComponent<SceneComponent>();
	pCamHolder->AttachTo(GetRoot());
	pCamHolder->localTransform.position = Vector3(0.0f, 1.5f, 0.0f);

	std::shared_ptr<CameraComponent> pCamComp = AddComponent<CameraComponent>();
	pCamComp->RegisterMainCamera();

	pCamComp->AttachTo(pCamHolder);
	pCamComp->localTransform.position = Vector3(0.0f, 0.0f, -5.0f);

	m_camHolder = pCamHolder;
}

void CharacterController::LateTick(float _dt)
{
	Controller::LateTick(_dt);

	FollowPawn(_dt);
	RotateCamera(_dt);
}

void CharacterController::OnPossessed(Input& _input)
{
	Controller::OnPossessed(_input);

	_input.GetKeyBind(DirectX::Keyboard::Keys::Escape).OnPressed = std::bind([this]() { PostQuitMessage(0); });
	_input.GetKeyBind(DirectX::Keyboard::Keys::W).OnPressed = std::bind(
		[this]()
		{
			std::shared_ptr<Character> pChar = GetChar();
			if (!pChar)
				return;

			Vector2 inputDir = pChar->GetInputDir();
			inputDir.y = 1.0f;
			pChar->SetInputDir(inputDir);
		});
	_input.GetKeyBind(DirectX::Keyboard::Keys::W).OnReleased = std::bind(
		[this]()
		{
			std::shared_ptr<Character> pChar = GetChar();
			if (!pChar)
				return;

			Vector2 inputDir = pChar->GetInputDir();
			inputDir.y = 0.0f;
			pChar->SetInputDir(inputDir);
		});

	_input.GetKeyBind(DirectX::Keyboard::Keys::S).OnPressed = std::bind(
		[this]()
		{
			std::shared_ptr<Character> pChar = GetChar();
			if (!pChar)
				return;

			Vector2 inputDir = pChar->GetInputDir();
			inputDir.y = -1.0f;
			pChar->SetInputDir(inputDir);
		});

	_input.GetKeyBind(DirectX::Keyboard::Keys::S).OnReleased = std::bind(
		[this]()
		{
			std::shared_ptr<Character> pChar = GetChar();
			if (!pChar)
				return;

			Vector2 inputDir = pChar->GetInputDir();
			inputDir.y = 0.0f;
			pChar->SetInputDir(inputDir);
		});

	_input.GetKeyBind(DirectX::Keyboard::Keys::D).OnPressed = std::bind(
		[this]()
		{
			std::shared_ptr<Character> pChar = GetChar();
			if (!pChar)
				return;

			Vector2 inputDir = pChar->GetInputDir();
			inputDir.x = 1.0f;
			pChar->SetInputDir(inputDir);
		});
	_input.GetKeyBind(DirectX::Keyboard::Keys::D).OnReleased = std::bind(
		[this]()
		{
			std::shared_ptr<Character> pChar = GetChar();
			if (!pChar)
				return;

			Vector2 inputDir = pChar->GetInputDir();
			inputDir.x = 0.0f;
			pChar->SetInputDir(inputDir);
		});

	_input.GetKeyBind(DirectX::Keyboard::Keys::A).OnPressed = std::bind(
		[this]()
		{
			std::shared_ptr<Character> pChar = GetChar();
			if (!pChar)
				return;

			Vector2 inputDir = pChar->GetInputDir();
			inputDir.x = -1.0f;
			pChar->SetInputDir(inputDir);
		});
	_input.GetKeyBind(DirectX::Keyboard::Keys::A).OnReleased = std::bind(
		[this]()
		{
			std::shared_ptr<Character> pChar = GetChar();
			if (!pChar)
				return;

			Vector2 inputDir = pChar->GetInputDir();
			inputDir.x = 0.0f;
			pChar->SetInputDir(inputDir);
		});

	// 테스트용 점프
	_input.GetKeyBind(DirectX::Keyboard::Keys::Space).OnReleased = 
		[this]()
		{
			std::shared_ptr<Character> pChar = GetChar();
			if (!pChar)
				return;

			pChar->InputJump();
		};
	_input.GetKeyBind(DirectX::Keyboard::Keys::LeftShift).OnPressed = 
		[this]()
		{
			std::shared_ptr<Character> pChar = GetChar();
			if (!pChar)
				return;

			pChar->TryPerception();
		};
	_input.GetKeyBind(DirectX::Keyboard::Keys::F3).OnPressed =
		[this]()
		{
			ResetCamRot();
		};
	_input.GetKeyBind(DirectX::Keyboard::Keys::Q).OnPressed =
		[this]()
		{
			std::shared_ptr<Character> pChar = GetChar();
			if (!pChar)
				return;

			pChar->SetPosition(Vector3(0.0f));
		};
}

std::shared_ptr<Character> CharacterController::GetChar()
{
	return std::dynamic_pointer_cast<Character>(GetPossessed());
}

void CharacterController::FollowPawn(float _dt)
{
	if (!IsPossessing())
		return;

	Vector3& pos = GetRoot()->localTransform.position;
	pos = Vector3::Lerp(pos, GetChar()->GetRoot()->localTransform.position, m_followLerpWeight);
}

void CharacterController::RotateCamera(float _dt)
{
	Input& input = InputManager::GetInstance()->GetInput();

	// 마우스 델타에 이미 델타타임이 곱해져 있음
	const Vector2 camRotSpeed = m_camRotateSpeed * input.GetMouseDelta();

	m_camRotate.x += camRotSpeed.x;
	if (m_camRotate.x < 0.0f)
		m_camRotate.x += 360.0f;
	else if (m_camRotate.x > 360.0f)
		m_camRotate.x -= 360.0f;

	m_camRotate.y += camRotSpeed.y;
	m_camRotate.y = std::clamp(m_camRotate.y, -m_camPitchMaxDeg, m_camPitchMaxDeg);

	Quaternion& camHolderRot = m_camHolder.lock()->localTransform.rotation;
	camHolderRot = Quaternion::CreateFromYawPitchRoll(0.0f, ToRadians(m_camRotate.y), 0.0f);
	camHolderRot.Normalize();

	Quaternion& contRot = GetRoot()->localTransform.rotation;
	contRot = Quaternion::CreateFromYawPitchRoll(ToRadians(m_camRotate.x), 0.0f, 0.0f);
	contRot.Normalize();
}
