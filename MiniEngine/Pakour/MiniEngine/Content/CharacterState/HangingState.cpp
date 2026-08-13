#include "pch.h"
#include "Content/CharacterState/HangingState.h"

#include "Platform/Input.h"
#include "Scene/Scene.h"
#include "Perception/Interface/IObstacle.h"
#include "Perception/Config/ObstacleConfig.h"

#include "Content/Character.h"
#include "Content/Data/CharacterConfigData.h"

#include "Core/Log.h"
#include "Core/DebugMarkers.h"

using namespace Content::Config;
using namespace MiniEngine::Physics;
using namespace MiniEngine;

void HangingState::OnStart()
{
	std::shared_ptr<Character> pChar = GetMachine()->GetCharacter();
	pChar->SetUseGravity(false); // 매달린 중에는 중력 적용 해제
	pChar->TranstionBaseTrack(static_cast<uint8_t>(pChar->GetState()), 0.2f);

	pChar->ReserveIKDetectWall();
	pChar->SetIKPoleVector((uint8_t)ELimbType::LeftArm,		Vector3(-1.0f, -1.0f, -0.5f));
	pChar->SetIKPoleVector((uint8_t)ELimbType::RightArm,	Vector3(1.0f, -1.0f, -0.5f));

	m_pCurrentObstacle = pChar->GetCurObstacleInfo().m_pObstacle;
	AlignToNormal();
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

	if (tag == (uint8_t)ETagEnvDetail::Protrude)
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
	
	MG_LOG_INFO("[HangingState::AlignToNormal] : ({:.2f}, {:.2f}, {:.2f})", nrm.x, nrm.y, nrm.z);

	Quaternion rot;
	if (TryYawRotateToward(nrm, rot))
		pChar->GetRoot()->localTransform.rotation = rot;
}


void PoleHangingState::OnStart()
{
	std::shared_ptr<Character> pChar = GetMachine()->GetCharacter();
	pChar->SetUseGravity(false); // 매달린 중에는 중력 적용 해제
	pChar->TranstionBaseTrack(static_cast<uint8_t>(pChar->GetState()), 0.2f);
	pChar->ReserveIKDetectPole();

	Refresh();
}

void PoleHangingState::OnEnd()
{
	std::shared_ptr<Character> pChar = GetMachine()->GetCharacter();
	pChar->SetUseGravity(true); // 매달림 해제
	pChar->ClearIKReserve();

	ClearCurObstacle();
}

void PoleHangingState::AlignToNormal()
{
	std::shared_ptr<Character> pChar = GetMachine()->GetCharacter();

	uint8_t subInfo = 0;
	if (pChar->GetCurObstacleInfo().m_pObstacle->TryGetTag(TAG_SUB_INFO, subInfo) == false)
	{
		AlignDefault();
		return;
	}

	AlighAxis(subInfo);
}


void PoleHangingState::AlignDefault()
{
	std::shared_ptr<Character> pChar = GetMachine()->GetCharacter();
	const Vector3 CHAR_FWD = pChar->GetRoot()->localTransform.Forward();
	const PerceptedObstacleInfo& CUR_OBS = pChar->GetCurObstacleInfo();
	const Transform& OBS_TF = CUR_OBS.m_pObstacle->GetTransform();

	const std::vector<Vector3> DIR
	{
		OBS_TF.Forward(),
		OBS_TF.Right(),
		-OBS_TF.Forward(),
		-OBS_TF.Right()
	};

	Vector3 nearestNrm(0.0f);
	float minDot = FLT_MAX;

	for (const Vector3& dir : DIR)
	{
		float dot = dir.Dot(CHAR_FWD);
		if (minDot > dot)
		{
			minDot = dot;
			nearestNrm = dir;
		}
	}
	//

	nearestNrm *= -1.0f;

	Quaternion rot;
	if (TryYawRotateToward(nearestNrm, rot))
		pChar->GetRoot()->localTransform.rotation = rot;
}

void PoleHangingState::AlighAxis(uint8_t _axis)
{
	std::shared_ptr<Character> pChar = GetMachine()->GetCharacter();
	const Transform& OBS_TF = pChar->GetCurObstacle()->GetTransform();
	Vector3 dir = 
		(ETagAxis)_axis == ETagAxis::X ? 
		OBS_TF.Right() : OBS_TF.Forward();

	const Vector3& CHAR_FWD = pChar->GetRoot()->localTransform.Forward();
	float dot = dir.Dot(CHAR_FWD);

	if (dot < 0.0f)
		dir *= -1.0f;

	Quaternion rot;
	if (TryYawRotateToward(dir, rot))
		pChar->GetRoot()->localTransform.rotation = rot;
}