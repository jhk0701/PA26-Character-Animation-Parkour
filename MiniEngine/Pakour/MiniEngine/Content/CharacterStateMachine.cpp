#include "pch.h"
#include "Content/CharacterStateMachine.h"
#include "Content/Character.h"
#include "Platform/Input.h"
#include "Content/ContentConfig.h"

using namespace Content::Config;

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


void CharacterState::DefaultMovement(float _dt)
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
void CharacterState::DefaultCameraRotate(float _dt)
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


void LandingState::OnStart(){}
void LandingState::OnEnd()
{
	GetMachine()->GetCharacter()->InputLerp() = Vector2(0.0f);
}

void LandingState::Tick(float _dt)
{
	DefaultMovement(_dt);
	DefaultCameraRotate(_dt);

	CheckState();
}

void LandingState::CheckState()
{
	std::shared_ptr<Character> pChar = GetMachine()->GetCharacter();

	if (pChar->IsFalling() == false)
		return;

	// 떨어지는 중
	pChar->SetState(Character::EState::InAir);
	const uint8_t STATE = (uint8_t)pChar->GetState();

	pChar->TranstionBaseTrack(STATE);
	GetMachine()->Transition(STATE);
}

void InAirState::OnStart() {}
void InAirState::OnEnd() {}
void InAirState::Tick(float _dt)
{
	// 떨어지는 중엔 movement 막기
	DefaultCameraRotate(_dt);

	CheckState();
}


void InAirState::CheckState()
{
	// 공중 + 떨어지는 상황
	// 바닥 감지 필요
	std::shared_ptr<Character> pChar = GetMachine()->GetCharacter();
	if (pChar->IsGrounded() == false)
		return;

	// 공중 -> 착지 모션
	if (std::shared_ptr<ActionClip> pClip = pChar->GetActions((uint8_t)Content::Config::ETagAct::FallingToLand))
		pChar->PlayActionClip(pClip, 0.2f);

	pChar->SetState(Character::EState::Landing);
	const uint8_t STATE = (uint8_t)pChar->GetState();

	pChar->TranstionBaseTrack(STATE, 0.25f);
	GetMachine()->Transition(STATE);
}

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
	CheckState();
}

void HangingState::CheckState()
{
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
		eAct = ETagAct::HangingMoveUp;
	else if (INPUT_DIR.y < 0)
		eAct = ETagAct::HangingMoveDown;
	else if (INPUT_DIR.x < 0)
		eAct = ETagAct::HangingMoveRight;
	else if (INPUT_DIR.x > 0)
		eAct = ETagAct::HangingMoveLeft;

	if (std::shared_ptr<ActionClip> pAct = pChar->GetActions((uint8_t)eAct))
		pChar->PlayActionClip(pAct, 0.1f);
}
