#include "pch.h"
#include "Content/CharacterState/HangingState.h"
#include "Platform/Input.h"
#include "Physics/PhysicsWorld.h"
#include "Scene/Scene.h"
#include "Perception/Interface/IObstacle.h"
#include "Content/Character.h"
#include "Content/Data/CharacterConfigData.h"

#include "Core/Log.h"
#include "Core/DebugMarkers.h"

using namespace Content::Config;
using namespace MiniEngine::Physics;

void HangingState::OnStart()
{
	std::shared_ptr<Character> pChar = GetMachine()->GetCharacter();
	pChar->SetUseGravity(false); // 매달린 중에는 중력 적용 해제
	pChar->TranstionBaseTrack(static_cast<uint8_t>(pChar->GetState()), 0.2f);
	pChar->ReserveIKDetectWall();

	pChar->SetIKPoleVector((uint8_t)ELimbType::LeftArm,		Vector3(-1.0f, -1.0f, -0.5f));
	pChar->SetIKPoleVector((uint8_t)ELimbType::RightArm,	Vector3(1.0f, -1.0f, -0.5f));

	Refresh();
}

void HangingState::OnEnd()
{
	std::shared_ptr<Character> pChar = GetMachine()->GetCharacter();
	pChar->SetUseGravity(true); // 매달림 해제

	ClearCurObstacle();
	pChar->ClearIKReserve();

	pChar->SetIKPoleVector((uint8_t)ELimbType::LeftArm,		Vector3(-1.0f, 0.0f, -1.0f));
	pChar->SetIKPoleVector((uint8_t)ELimbType::RightArm,	Vector3(1.0f, 0.0f, -1.0f));
}

void HangingState::Tick(float _dt) {}
void HangingState::LateTick(float _dt) {}

void HangingState::Refresh()
{
	CharacterState::Refresh();

	std::shared_ptr<Character> pChar = GetMachine()->GetCharacter();
	m_pCurrentObstacle = pChar->GetCurObstacleInfo().m_pObstacle;

	uint8_t tag = 0;
	m_pCurrentObstacle->TryGetTag(TAG_ENV_DETAIL, tag);

	if (tag == (uint8_t)ETagEnvDetail::Protrude || (uint8_t)ETagEnvDetail::Pole)
		return;
	
	AlignToNormal();
}

IObstacle* HangingState::GetCurrentObstacle() const
{
	return m_pCurrentObstacle;
}

void HangingState::AlignToNormal()
{
	std::shared_ptr<Character> pChar = GetMachine()->GetCharacter();

	Vector3 nrm = -pChar->GetCurObstacleInfo().m_obstacleHitNrm;
	nrm.y = 0.0f;
	nrm.Normalize();

	// MG_LOG_INFO("[HangingState::AlignToNormal] : ({:.2f}, {:.2f}, {:.2f})", nrm.x, nrm.y, nrm.z);

	Quaternion rot;
	if (TryYawRotateToward(nrm, rot))
		pChar->GetRoot()->localTransform.rotation = rot;
}


void PoleHangingState::OnStart()
{
	std::shared_ptr<Character> pChar = GetMachine()->GetCharacter();
	pChar->SetUseGravity(false); // 매달린 중에는 중력 적용 해제
	pChar->TranstionBaseTrack(static_cast<uint8_t>(pChar->GetState()), 0.2f);

	Refresh();
}

void PoleHangingState::OnEnd()
{
	std::shared_ptr<Character> pChar = GetMachine()->GetCharacter();
	pChar->SetUseGravity(true); // 매달림 해제

	ClearCurObstacle();
}
