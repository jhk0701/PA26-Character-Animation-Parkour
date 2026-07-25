#include "pch.h"
#include "Platform/Input.h"
#include "Content/CharacterController.h"
#include "Content/Character.h"

using namespace MiniEngine;

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
	_input.GetKeyBind(DirectX::Keyboard::Keys::Space).OnReleased = std::bind(
		[this]() 
		{ 
			std::shared_ptr<Character> pChar = GetChar();
			if (!pChar)
				return;

			pChar->InputJump();
		}
	);
	_input.GetKeyBind(DirectX::Keyboard::Keys::LeftShift).OnPressed = std::bind(
		[this]() 
		{ 
			std::shared_ptr<Character> pChar = GetChar();
			if (!pChar)
				return;

			pChar->TryPerception();
		}
	);
	_input.GetKeyBind(DirectX::Keyboard::Keys::F3).OnPressed = std::bind(
		[this]() 
		{ 
			std::shared_ptr<Character> pChar = GetChar();
			if (!pChar)
				return;

			pChar->ResetCamRot();
		}
	);
	_input.GetKeyBind(DirectX::Keyboard::Keys::Q).OnPressed = std::bind(
		[this]()
		{
			std::shared_ptr<Character> pChar = GetChar();
			if (!pChar)
				return;

			pChar->SetPosition(Vector3(0.0f));
		}
	);
}

std::shared_ptr<Character> CharacterController::GetChar()
{
	return std::dynamic_pointer_cast<Character>(GetPossessed());
}
