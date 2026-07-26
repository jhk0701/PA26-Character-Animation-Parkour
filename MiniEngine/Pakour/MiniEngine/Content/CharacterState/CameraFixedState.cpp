#include "pch.h"
#include "Content/CharacterState/CameraFixedState.h"
#include "Content/Character.h"
#include "Platform/Input.h"

void CameraFixedState::OnStart()
{
	std::shared_ptr<Character> pChar = GetMachine()->GetCharacter();
}

void CameraFixedState::OnEnd()
{
	std::shared_ptr<Character> pChar = GetMachine()->GetCharacter();
}
