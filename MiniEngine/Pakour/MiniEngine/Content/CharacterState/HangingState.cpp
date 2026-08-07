#include "pch.h"
#include "Content/CharacterState/HangingState.h"
#include "Platform/Input.h"
#include "Physics/PhysicsWorld.h"
#include "Scene/Scene.h"
#include "Scene/IObstacle.h"
#include "Content/Character.h"
#include "Content/Data/CharacterPerceptionConfig.h"

#include "Core/Log.h"
#include "Core/DebugMarkers.h"

using namespace Content::Config;
using namespace MiniEngine::Physics;

void HangingState::OnStart()
{
	std::shared_ptr<Character> pChar = GetMachine()->GetCharacter();
	pChar->SetUseGravity(false); // 매달린 중에는 중력 적용 해제
	pChar->TranstionBaseTrack(static_cast<uint8_t>(pChar->GetState()), 0.25f);
	pChar->ReserveIKDetectWall();

	Refresh();
}

void HangingState::OnEnd()
{
	std::shared_ptr<Character> pChar = GetMachine()->GetCharacter();
	pChar->SetUseGravity(true); // 매달림 해제

	m_pCurrentObstacle = nullptr;
	pChar->ClearIKReserve();
}

void HangingState::Tick(float _dt) {}

void HangingState::LateTick(float _dt){ }

void HangingState::Refresh()
{
	CharacterState::Refresh();

	std::shared_ptr<Character> pChar = GetMachine()->GetCharacter();
	m_pCurrentObstacle = pChar->GetCurObstacleInfo().m_pObstacle;

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

	Quaternion rot;
	if (TryYawRotateToward(nrm, rot))
		pChar->GetRoot()->localTransform.rotation = rot;
}