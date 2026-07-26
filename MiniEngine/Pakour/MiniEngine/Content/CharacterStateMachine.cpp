#include "pch.h"
#include "Content/CharacterStateMachine.h"

#include "Content/ContentConfig.h"
#include "Content/Character.h"
#include "Content/CharacterController.h"
#include "Platform/Input.h"

#include "Core/Log.h"

void CharacterStateMachine::RegisterStates(std::vector<std::shared_ptr<CharacterState>>&& _states)
{
	m_states = _states;
	for (std::shared_ptr<CharacterState>& pState : m_states)
		pState->RegisterMachine(shared_from_this());
	
	assert(m_states.size() > 0); // false 중단
	Transition(0);
}

void CharacterStateMachine::Tick(float _dt)
{
	Component::Tick(_dt);
	m_states[m_curState]->Tick(_dt);
}

void CharacterStateMachine::Transition(uint8_t _nextID)
{
	assert(_nextID < m_states.size());

	if (m_curState == _nextID)
	{
		m_states[m_curState]->Refresh();
		return;
	}

	m_states[m_curState]->OnEnd();

	m_curState = _nextID;

	m_states[m_curState]->OnStart();
}

void CharacterStateMachine::ProcessPerceptionResult(const Character::PerceptedObstacleInfo& _info)
{
	assert(m_curState < m_states.size());
	m_states[m_curState]->ProcessPerceptionResult(_info);
}

std::shared_ptr<Character> CharacterStateMachine::GetCharacter()
{
	return std::dynamic_pointer_cast<Character>(owner.lock());
}

void CharacterState::DefaultProcessPerceptionResult(const Character::PerceptedObstacleInfo& _info)
{
	std::shared_ptr<Character> pChar = GetMachine()->GetCharacter();

	if (std::shared_ptr<ActionClip> pAction = pChar->GetActions(_info.m_actTag))
		pChar->PlayActionClip(pAction, 0.2f, (uint8_t)Content::Config::EActionPriority::Override);
}

void CharacterState::ProcessMovement(float _dt)
{
	std::shared_ptr<Character> pChar = GetMachine()->GetCharacter();
	if (pChar->IsActionClipPlaying())
		return;

	const Vector2& INPUT_DIR = pChar->GetInputDir();

	Vector2& inputLerp = pChar->InputLerp();
	inputLerp = Vector2::Lerp(inputLerp, INPUT_DIR, pChar->GetInputLerpWeight());
	
	if (INPUT_DIR.x != 0 || INPUT_DIR.y != 0)
		inputLerp.Normalize();
	else 
	{
		pChar->SetAnimBaseTrackInputAxis(inputLerp);
		return;
	}
	
	const float DELTA_SPD = _dt * pChar->GetMoveSpeed();
	const Transform& CONT_TF = pChar->GetControllerActor()->GetRoot()->localTransform;
	
	pChar->AddMovementInput(
		DELTA_SPD * inputLerp.y * CONT_TF.Forward() +
		DELTA_SPD * inputLerp.x * CONT_TF.Right()
	);
	pChar->SetAnimBaseTrackInputAxis(inputLerp);
}

void CharacterState::SyncControllerRotate()
{
	std::shared_ptr<Character> pChar = GetMachine()->GetCharacter();
	if (pChar->IsActionClipPlaying())
		return;

	pChar->GetRoot()->localTransform.rotation = pChar->GetControllerActor()->GetRoot()->localTransform.rotation;
}
