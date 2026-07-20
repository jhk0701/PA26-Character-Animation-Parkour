#include "pch.h"
#include "Content/CharacterStateMachine.h"
#include "Content/Character.h"

CharacterState::CharacterState()
{
}

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

	m_states[m_curState]->OnEnd();

	m_curState = _nextID;

	m_states[m_curState]->OnStart();
}

std::shared_ptr<Character> CharacterStateMachine::GetCharacter()
{
	return std::dynamic_pointer_cast<Character>(owner.lock());
}


void LandingState::OnStart()
{
}

void LandingState::Tick(float _dt)
{
	InputMovement(_dt);
	InputCamRotate(_dt);
}

void LandingState::OnEnd()
{
}

void LandingState::InputMovement(float _dt)
{
	std::shared_ptr<Character> pChar = GetMachine()->GetCharacter();
	if (pChar->IsActionClipPlaying())
		return;

	Vector2 inputDir = pChar->GetInputDir();
	inputDir.Normalize();

	Vector2& inputLerp = pChar->InputLerp();
	inputLerp = Vector2::Lerp(inputLerp, inputDir, pChar->GetInputLerpWeight() * _dt);

	const float deltaSpeed = _dt * pChar->GetMoveSpeed();
	std::shared_ptr<SceneComponent> pRoot = pChar->GetRoot();

	// 캐릭터 정면 기준 이동
	const Vector3& fwd = pRoot->localTransform.Forward();
	const Vector3& rht = pRoot->localTransform.Right();
	pChar->AddMovementInput(deltaSpeed * inputLerp.y * fwd + deltaSpeed * -inputLerp.x * rht);

	pChar->SetAnimBaseTrackInputAxis(inputLerp);
}

void LandingState::InputCamRotate(float _dt)
{
}


void HangingState::OnStart()
{
}

void HangingState::Tick(float _dt)
{
}

void HangingState::OnEnd()
{
}


void InAirState::OnStart()
{
}

void InAirState::Tick(float _dt)
{
}

void InAirState::OnEnd()
{
}
