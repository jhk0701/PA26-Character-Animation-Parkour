#include "pch.h"
#include "Scene/Controller.h"
#include "Scene/Pawn.h"
#include "Scene/SceneComponent.h"
#include "Platform/Input.h"

using namespace MiniEngine;

void Controller::Construct()
{
	std::shared_ptr<SceneComponent> pRoot = AddComponent<SceneComponent>();
}

void Controller::Possess(std::shared_ptr<Pawn> _pawn)
{
	m_possesed = _pawn;
	OnPossessed(InputManager::GetInstance()->GetInput());
}

void Controller::Unpossess()
{
	m_possesed.reset();
	InputManager::GetInstance()->GetInput().Clear(); // 지금은 입력으로 컨트롤할 객체가 하나라고 가정
}

void Controller::OnPossessed(Input& _input)
{
	m_possesed.lock()->OnPossessed(std::static_pointer_cast<Controller>(shared_from_this()), _input);
}
