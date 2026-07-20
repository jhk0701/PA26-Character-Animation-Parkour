#include "pch.h"
#include "Content/CharacterStateMachine.h"
#include "Content/Character.h"
#include "Platform/Input.h"

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


void LandingState::OnStart(){}
void LandingState::OnEnd()
{
	GetMachine()->GetCharacter()->InputLerp() = Vector2(0.0f);
}

void LandingState::Tick(float _dt)
{
	InputMovement(_dt);
	InputCamRotate(_dt);

	CheckState();
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

	const float DELTA_SPD = _dt * pChar->GetMoveSpeed();
	std::shared_ptr<SceneComponent> pRoot = pChar->GetRoot();

	// 캐릭터 정면 기준 이동
	const Vector3& fwd = pRoot->localTransform.Forward();
	const Vector3& rht = pRoot->localTransform.Right();
	pChar->AddMovementInput(DELTA_SPD * inputLerp.y * fwd + DELTA_SPD * -inputLerp.x * rht);

	pChar->SetAnimBaseTrackInputAxis(inputLerp);
}

void LandingState::InputCamRotate(float _dt)
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

	Quaternion qYaw = Quaternion::CreateFromAxisAngle(Vector3::Transform(Vector3(.0f, 1.0f, .0f), Quaternion(0.0f, 0.0f, 0.0f, 1.0f)), ToRadians(camRot.x));
	Quaternion qPitch = Quaternion::CreateFromAxisAngle(Vector3(1.0f, 0.0f, 0.0f), ToRadians(camRot.y));
	qYaw.Normalize();
	qPitch.Normalize();

	pChar->GetRoot()->localTransform.rotation = qYaw;

	std::shared_ptr<SceneComponent> pCamHolderRoot = pChar->GetCamHolder().lock();
	pCamHolderRoot->localTransform.rotation = qPitch;
}

void LandingState::CheckState()
{
	std::shared_ptr<Character> pChar = GetMachine()->GetCharacter();

	if (pChar->IsFalling() == false)
		return;

	// 떨어지는 중
	pChar->SetState(Character::EState::InAir);

	uint8_t state = (uint8_t)pChar->GetCharState();
	pChar->TranstionBaseTrack(state, 0.25f);
	GetMachine()->Transition(state);
}

void InAirState::OnStart()
{
}

void InAirState::OnEnd()
{
}

void InAirState::Tick(float _dt)
{
}

void HangingState::OnStart()
{
}

void HangingState::OnEnd()
{
}

void HangingState::Tick(float _dt)
{
}
