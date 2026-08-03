#include "pch.h"
#include "Content/CharacterState/InAirState.h"
#include "Content/Character.h"
#include "Content/ContentConfig.h"
#include "Core/Log.h"

using namespace Content::Config;

void InAirState::OnStart() 
{
	std::shared_ptr<Character> pChar = GetMachine()->GetCharacter();
	pChar->TranstionBaseTrack(static_cast<uint8_t>(pChar->GetState()), 0.25f);
}

void InAirState::OnEnd() 
{
	// 강제로 떨어지는 경우?
}

void InAirState::Tick(float _dt)
{
	CheckState();
}

void InAirState::LateTick(float _dt)
{
}

void InAirState::CheckState()
{
	CharacterState::CheckState();

	// 공중 + 떨어지는 상황
	// 바닥 감지 필요
	std::shared_ptr<Character> pChar = GetMachine()->GetCharacter();
	if (pChar->IsGrounded() == false)
		return;

	// 공중 -> 착지 모션
	if (std::shared_ptr<ActionClip> pClip = pChar->GetActions((uint8_t)ETagAct::FallingToLand))
		pChar->PlayActionClip(pClip, 0.2f);

	pChar->SetState(Character::EState::Landing);
	const uint8_t STATE = (uint8_t)pChar->GetState();

	// 각각의 State에서 OnStart 시, 실행해 줄 것
	// pChar->TranstionBaseTrack(STATE, 0.25f);
	GetMachine()->Transition(STATE);
}

void InAirState::ProcessPerceptionResult(const Character::PerceptedObstacleInfo& _info)
{
	uint8_t tag = 0;
	if (_info.m_pObstacle->TryGetTag((uint8_t)TAG_ENV_DETAIL, tag) == false ||
		tag == (uint8_t)ETagEnvDetail::Beam || 
		tag == (uint8_t)ETagEnvDetail::Protrude)
	{
		DefaultProcessPerceptionResult(_info);
		return;
	}

	ProcessAirToDefault(_info);
}

void InAirState::ProcessAirToDefault(const Character::PerceptedObstacleInfo& _info)
{
	std::shared_ptr<Character> pChar = GetMachine()->GetCharacter();
	const PerceptionConfig& CONFIG = pChar->GetPerceptionConfig();

	const float HEIGHT_DIFF = _info.m_obstacleLedge - pChar->GetRoot()->localTransform.position.y;

	if (HEIGHT_DIFF > 0.0f) 
	{
		uint8_t act = 0;
		if (HEIGHT_DIFF >= CONFIG.thresholdWallHeight)
			act = (uint8_t)ETagAct::Wall_AirToHang;
		else
			act = (uint8_t)(_info.m_obstacleDepth >= CONFIG.minMantleDepth ? ETagAct::MantleAirToAttach : ETagAct::VaultAirToAttach);

		if(std::shared_ptr<ActionClip> pAction = pChar->GetActions(act))
			pChar->PlayActionClip(pAction, 0.2, (uint8_t)EActionPriority::Override);
	}
}
