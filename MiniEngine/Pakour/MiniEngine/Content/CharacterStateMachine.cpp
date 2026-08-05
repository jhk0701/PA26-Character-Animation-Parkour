#include "pch.h"
#include "Content/CharacterStateMachine.h"

#include "Content/ContentConfig.h"
#include "Content/Character.h"
#include "Content/CharacterController.h"
#include "Scene/IObstacle.h"
#include "Content/Data/CharacterPerceptionConfig.h"

#include "Platform/Input.h"

#include "Core/Log.h"

using namespace Content::Config;

void CharacterState::DefaultProcessPerceptionResult(const Character::PerceptedObstacleInfo& _info)
{
	uint8_t type;
	if (_info.m_pObstacle->TryGetTag(Content::Config::TAG_ENV_DETAIL, type) == false)
	{
		ProcessDefaultObstacle(_info);
		return;
	}

	switch ((ETagEnvDetail)type)
	{
	case ETagEnvDetail::Beam:
		ProcessBeamObstacle(_info);
		break;
	case ETagEnvDetail::Protrude:
		ProcessProstrudeObstacle(_info);
		break;
	case ETagEnvDetail::Default: __fallthrough;
	default:
		ProcessDefaultObstacle(_info);
		break;
	}
}

void CharacterState::ProcessDefaultObstacle(const Character::PerceptedObstacleInfo& _info)
{
	std::shared_ptr<Character> pChar = GetMachine()->GetCharacter();
	const PerceptionConfig& CONFIG = pChar->GetPerceptionConfig();

	uint8_t actTag =
		(uint8_t)(_info.m_obstacleDepth >= CONFIG.minMantleDepth ? ETagAct::Mantle : ETagAct::Vault);

	const float DIFF_HEIGHT = _info.m_obstacleLedge - pChar->GetRoot()->localTransform.position.y;
	if (DIFF_HEIGHT < .0f)
		return;

	// 올라가야함
	if (DIFF_HEIGHT >= CONFIG.thresholdWallHeight)
		actTag = (uint8_t)ETagAct::Wall;
	else if (DIFF_HEIGHT >= CONFIG.thresholdHighObstacle)
		actTag += 2;
	else if (DIFF_HEIGHT >= CONFIG.thresholdLowObstacle)
		actTag += 1;

	if (std::shared_ptr<ActionClip> pAction = pChar->GetActions(actTag))
		pChar->PlayActionClip(pAction, 0.2f, (uint8_t)EActionPriority::Override);
}

void CharacterState::ProcessBeamObstacle(const Character::PerceptedObstacleInfo& _info)
{
	// 확인한 대상이 Beam
	// 높이 확인 필요
	// Ledge의 위치가 캐릭터의 절반 높이 확인

	std::shared_ptr<Character> pChar = GetMachine()->GetCharacter();
	
	float charY = pChar->GetRoot()->localTransform.position.y + pChar->GetCharacterHalfHeight();
	uint8_t actTag = _info.m_obstacleLedge < charY ? (uint8_t)ETagAct::BeamStand : (uint8_t)ETagAct::BeamHanging;

	if (std::shared_ptr<ActionClip> pAction = pChar->GetActions(actTag))
		pChar->PlayActionClip(pAction, 0.2f, (uint8_t)EActionPriority::Override);
}

void CharacterState::ProcessProstrudeObstacle(const Character::PerceptedObstacleInfo& _info)
{
	MG_LOG_INFO("[CharacterState::ProcessProstrudeObstacle] Process Protrude" );

	// 벽면 돌출부 protrude
	// 해당 위치로 매달리기
	std::shared_ptr<Character> pChar = GetMachine()->GetCharacter();

	if(std::shared_ptr<ActionClip> pAction = pChar->GetActions((uint8_t)ETagAct::Wall_AirToHang))
		pChar->PlayActionClip(pAction, 0.2f, (uint8_t)EActionPriority::Override);
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

	const Vector2& INPUT_DIR = pChar->GetInputDir();
	if (INPUT_DIR.LengthSquared() > 0.0f)
		pChar->GetRoot()->localTransform.rotation = pChar->GetControllerActor()->GetRoot()->localTransform.rotation;
}


void CharacterStateMachine::Start(uint8_t _startID)
{
	m_bInitialized = true;
	Transition(0);
}

void CharacterStateMachine::RegisterStates(std::vector<std::shared_ptr<CharacterState>>&& _states)
{
	m_states = _states;
	for (std::shared_ptr<CharacterState>& pState : m_states)
		pState->RegisterMachine(shared_from_this());
	
	assert(m_states.size() > 0); // false 중단
}

void CharacterStateMachine::Tick(float _dt)
{
	Component::Tick(_dt);
	m_states[m_curState]->Tick(_dt);
}

void CharacterStateMachine::LateTick(float _dt)
{
	Component::LateTick(_dt);
	m_states[m_curState]->LateTick(_dt);
}

void CharacterStateMachine::Transition(uint8_t _nextID)
{
	assert(_nextID < m_states.size());

	if (m_curState == _nextID)
	{
		m_states[m_curState]->Refresh();
		return;
	}

	if (m_curState >= 0)
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
