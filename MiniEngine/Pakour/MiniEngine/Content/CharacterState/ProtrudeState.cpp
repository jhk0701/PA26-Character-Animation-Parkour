#include "pch.h"
#include "Content/CharacterState/ProtrudeState.h"
#include "Content/ContentConfig.h"
#include "Core/Log.h"

using namespace Content::Config;
using namespace MiniEngine::Physics;

void ProtrudeState::OnStart()
{
	std::shared_ptr<Character> pChar = GetMachine()->GetCharacter();
	pChar->SetUseGravity(false); // 매달린 중에는 중력 적용 해제
	pChar->TranstionBaseTrack(static_cast<uint8_t>(pChar->GetState()), 0.25f);

	Refresh();
}

void ProtrudeState::OnEnd()
{
	std::shared_ptr<Character> pChar = GetMachine()->GetCharacter();
	pChar->SetUseGravity(true);  // 매달림 상태 해제 중력 적용
}

void ProtrudeState::Refresh()
{
	RotateFixState::Refresh();

	// 방향 고정
	AlignToNormal();
}

void ProtrudeState::Tick(float _dt){}

void ProtrudeState::LateTick(float _dt)
{
}

void ProtrudeState::ProcessPerceptionResult(const Character::PerceptedObstacleInfo& _result)
{
	DefaultProcessPerceptionResult(_result);
}

void ProtrudeState::AlignToNormal()
{
	std::shared_ptr<Character> pChar = GetMachine()->GetCharacter();

	if (!pChar)
		return;

	Vector3 nrm = -pChar->GetCurObstacleInfo().m_obstacleHitNrm;
	nrm.y = 0.0f;
	nrm.Normalize();

	Quaternion rot;
	if (TryYawRotateToward(nrm, rot))
		pChar->GetRoot()->localTransform.rotation = rot;
}
