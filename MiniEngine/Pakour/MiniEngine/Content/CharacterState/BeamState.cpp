#include "pch.h"
#include "Content/CharacterState/BeamState.h"
#include "Core/Log.h"

void BeamState::OnStart()
{
	// Beam 장애물에 snap된 상태
	std::shared_ptr<Character> pChar = GetMachine()->GetCharacter();
	pChar->TranstionBaseTrack((uint8_t)pChar->GetState());

	MG_LOG_INFO("[Character] Beam State Started");
	pChar->SetUseGravity(false);

	// Beam 상태 진입의 전제
	// Beam 태그가 달린 오브젝트를 처리하고 있을 것
	const Character::PerceptedObstacleInfo& OBS_INFO = pChar->GetCurObstacleInfo();
	assert(OBS_INFO.IsValid());

	MG_LOG_INFO("[Character] Beam Obs Position : ({},{},{})", OBS_INFO.m_obstacleHitPos.x, OBS_INFO.m_obstacleHitPos.y, OBS_INFO.m_obstacleHitPos.z);
	pChar->SetPosition(OBS_INFO.m_obstacleHitPos);
}

void BeamState::OnEnd()
{
	std::shared_ptr<Character> pChar = GetMachine()->GetCharacter();

	pChar->SetUseGravity(true);
}

void BeamState::Tick(float _dt){}
void BeamState::CheckState(){}
void BeamState::ProcessPerceptionResult(const Character::PerceptedObstacleInfo& _info)
{
	DefaultProcessPerceptionResult(_info);
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

	DefaultCameraRotate(_dt);
}

void BeamStandState::CheckState()
{
	BeamState::CheckState();
}

void BeamStandState::ProcessPerceptionResult(const Character::PerceptedObstacleInfo& _info)
{
	BeamState::ProcessPerceptionResult(_info);
}
