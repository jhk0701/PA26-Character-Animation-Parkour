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
}

void BeamState::OnEnd()
{
	CameraFixedState::OnEnd();

	std::shared_ptr<Character> pChar = GetMachine()->GetCharacter();

	pChar->SetUseGravity(true);
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

void BeamState::ProcessPerceptionResult(const Character::PerceptedObstacleInfo& _info)
{
	// DefaultProcessPerceptionResult(_info);
}

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
	else if (INPUT_DIR.y > 0) 
	{
		// y축 전방 입력 시, 앞으로 이동
		// 후방 이동은 모션이 없어 제외

		
	}

	pChar->SetAnimBaseTrackInputAxis(INPUT_DIR);
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
