#include "pch.h"
#include "Content/CharacterState/BeamState.h"
#include "Content/Character.h"
#include "Platform/Input.h"

#include "Core/Log.h"

using namespace Content::Config;

void BeamState::OnStart()
{
	CameraFixedState::OnStart();

	// Beam 장애물에 snap된 상태
	std::shared_ptr<Character> pChar = GetMachine()->GetCharacter();
	pChar->TranstionBaseTrack((uint8_t)pChar->GetState());

	MG_LOG_INFO("[Character] Beam State Started");
	pChar->SetUseGravity(false);

	// Beam 상태 진입의 전제
	// Beam 태그가 달린 오브젝트를 처리하고 있을 것
	const Character::PerceptedObstacleInfo& OBS_INFO = pChar->GetCurObstacleInfo();
	assert(OBS_INFO.IsValid());

	uint8_t subInfoTag;
	OBS_INFO.m_pObstacle->GetTag().GetTagAt(TAG_SUB_INFO, subInfoTag);

	m_curAxis = (ETagAxis)subInfoTag;
	m_pCurObs = OBS_INFO.m_pObstacle;

	OrientByAxis();
}

void BeamState::OnEnd()
{
	CameraFixedState::OnEnd();

	std::shared_ptr<Character> pChar = GetMachine()->GetCharacter();

	pChar->SetUseGravity(true);

	m_pCurObs = nullptr;
}

void BeamState::Tick(float _dt)
{
	CheckState();
	CameraRotate(_dt);
}

void BeamState::CheckState()
{
	CharacterState::CheckState();
}
void BeamState::ProcessPerceptionResult(const Character::PerceptedObstacleInfo& _info) {}

// Beam Stand
void BeamStandState::OnStart()
{
	BeamState::OnStart();
}

void BeamStandState::OnEnd()
{
	BeamState::OnEnd();
}

void BeamStandState::Tick(float _dt)
{
	BeamState::Tick(_dt);

	// DefaultCameraRotate(_dt);
	ProcessMovement(_dt);
}

void BeamStandState::CheckState()
{
	BeamState::CheckState();
}

void BeamStandState::ProcessPerceptionResult(const Character::PerceptedObstacleInfo& _info)
{
	BeamState::ProcessPerceptionResult(_info);
}

void BeamStandState::OrientByAxis()
{
	// 좁은 발판에 선 상황
	const Actor* pCurObs = GetCurObs();

	if (!pCurObs)
		return;

	std::shared_ptr<Character> pChar = GetMachine()->GetCharacter();
	Transform& charTF = pChar->GetRoot()->localTransform;

	Vector3 obsDir;
	switch (GetAxis())
	{
	case ETagAxis::X:
		obsDir = pCurObs->GetRoot()->localTransform.Right();
		break;
	case ETagAxis::Y:
		obsDir = pCurObs->GetRoot()->localTransform.Up();
		break;
	case ETagAxis::Z:
		obsDir = pCurObs->GetRoot()->localTransform.Forward();
		break;
	}

	// 발판을 전제로 함
	// y 방향으로 향하는 경우는 우선 배제
	obsDir.y = 0.0f;
	obsDir.Normalize();

	const Vector3 CHAR_FWD = charTF.Forward();
	const float DOT = fabs(CHAR_FWD.Dot(obsDir));// fabs();

	Vector3 toward;
	if (DOT >= 0.5f)
	{
		// Axis 방향으로 더 가까움
		// Axis 방향으로 향하도록 보정
		toward = obsDir;
	}
	else 
	{
		// Axis와 직교 방향으로 더 가까움
		// 직교 방향으로 향하도록 보정
		toward = CHAR_FWD;
	}

	charTF.rotation.LookRotation(toward, charTF.Up());
}

void BeamStandState::ProcessMovement(float _dt)
{
	std::shared_ptr<Character> pChar = GetMachine()->GetCharacter();
	if (pChar->IsActionClipPlaying())
		return;

	const Vector2& INPUT_DIR = pChar->GetInputDir();

	// x 축 입력 시 회전
	if (INPUT_DIR.x < 0)
	{
		if (std::shared_ptr<ActionClip> pAct = pChar->GetActions((uint8_t)ETagAct::Beam_StandRotateLeft))
			pChar->PlayActionClip(pAct, 0.1f);
		return;
	}
	else if (INPUT_DIR.x > 0) 
	{
		if (std::shared_ptr<ActionClip> pAct = pChar->GetActions((uint8_t)ETagAct::Beam_StandRotateRight))
			pChar->PlayActionClip(pAct, 0.1f);
		return;
	}
	
	// y축 입력 시, 앞 뒤로 이동
	// 후방 이동은 모션이 없어 제외 -> 블렌드 시, idle이 최종임
	Vector2& inputLerp = pChar->InputLerp();
	inputLerp = Vector2::Lerp(inputLerp, INPUT_DIR, pChar->GetInputLerpWeight());
	pChar->SetAnimBaseTrackInputAxis(inputLerp);

	if (inputLerp.y < 0)
		return;

	const float DELTA_SPD = _dt * pChar->GetMoveSpeed();
	const Transform& TF = pChar->GetRoot()->localTransform;

	pChar->AddMovementInput(DELTA_SPD * inputLerp.y * TF.Forward());
}


// Beam Hanging
void BeamHangingState::OnStart()
{
	BeamState::OnStart();
}

void BeamHangingState::OnEnd()
{
	BeamState::OnEnd();
}

void BeamHangingState::Tick(float _dt)
{
	BeamState::Tick(_dt);

	ProcessMovement(_dt);
}

void BeamHangingState::CheckState()
{
	BeamState::CheckState();
}

void BeamHangingState::ProcessPerceptionResult(const Character::PerceptedObstacleInfo& _info)
{
	BeamState::ProcessPerceptionResult(_info);
}

void BeamHangingState::OrientByAxis()
{
}

void BeamHangingState::ProcessMovement(float _dt)
{
	// 봉의 방향에 따라 좌우로만 이동
	std::shared_ptr<Character> pChar = GetMachine()->GetCharacter();
	if (pChar->IsActionClipPlaying())
		return;

	const Vector2 INPUT_DIR = pChar->GetInputDir();

	ETagAct eAct = ETagAct::End;
	if (INPUT_DIR.x > 0)
		eAct = ETagAct::Beam_HangingMoveRight;
	else if (INPUT_DIR.x < 0)
		eAct = ETagAct::Beam_HangingMoveLeft;

	if (std::shared_ptr<ActionClip> pAct = pChar->GetActions((uint8_t)eAct))
		pChar->PlayActionClip(pAct, 0.1f);
};
