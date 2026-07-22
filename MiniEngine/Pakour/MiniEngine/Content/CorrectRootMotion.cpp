#include "pch.h"
#include "Content/CorrectRootMotion.h"
#include "Content/Character.h"
#include "Scene/CharacterControllerComponent.h"
#include "Core/Log.h"

void CorrectRootMotion::OnStart(MiniEngine::AnimNotifyParam& _param)
{
	AnimNotifyState::OnStart(_param);

	if (!_param.m_pActor)
		return;

	m_pChar = dynamic_cast<Character*>(_param.m_pActor);
}

void CorrectRootMotion::Activate(float _dt, MiniEngine::AnimNotifyParam& _param)
{
	if (!m_pChar)
		return;

	const Character::PerceptedObstacleInfo& OBS_INFO = m_pChar->GetCurObstacleInfo();

	// 캐릭터와 장애물의 적정거리 보정
	Vector3 obsPos = OBS_INFO.m_obstacleHitPos;
	obsPos.y = OBS_INFO.m_obstacleLedge;
	Vector3 charPos = m_pChar->GetRoot()->localTransform.position;
	
	switch (m_corrextAxis)
	{
	case XZ:
		obsPos.y = 0.0f;
		charPos.y = 0.0f;
		break;
	case XY:
		obsPos.z = 0.0f;
		charPos.z = 0.0f;
		break;
	case YZ:
		obsPos.x = 0.0f;
		charPos.x = 0.0f;
		break;
	case None: __fallthrough;
	default:
		break;
	}
	
	Vector3 dir = obsPos - charPos;
	dir.Normalize();

	Vector3 properPoint = obsPos - dir * m_properDistance;
	Vector3 lerped = Vector3::Lerp(charPos, properPoint, m_lerpWeight);

	Vector3 correctMovementDt = lerped - charPos;
	
	// 멀 때만 보간 처리
	// 가까울 때도 처리하니, 진행방향에 역방향으로 움직여서 어색해보임
	if (OBS_INFO.m_obstacleDistance > m_properDistance)
		correctMovementDt *= _dt * m_deltaIntensity;

	if (m_bLockX)
		correctMovementDt.x = 0.0;
	if (m_bLockY)
		correctMovementDt.y = 0.0;
	if (m_bLockZ)
		correctMovementDt.z = 0.0;

	// MG_LOG_INFO("[CorrectRootMotion] : ({},{},{})", correctMovementDt.x, correctMovementDt.y, correctMovementDt.z);
	m_pChar->AddMovementInput(correctMovementDt);
}
