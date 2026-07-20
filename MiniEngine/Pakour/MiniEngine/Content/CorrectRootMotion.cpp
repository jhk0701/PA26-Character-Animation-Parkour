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

	MiniEngine::Actor* pCurObs = m_pChar->GetCurObstacle();
	if (pCurObs == nullptr)
		return;
	
	// 현재 장애물과 거리
	float dist = m_pChar->GetCurObstacleDistance(); // 지형탐색을 통해 얻은 첫 장애물로부터의 거리
	//MG_LOG_INFO("Correct dist : {}", dist);

	// 캐릭터와 장애물의 적정거리 보정
	Vector3 obsPos = m_pChar->GetCurObstacleHitPos(); //pCurObs->GetRoot()->localTransform.position;
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

		obsPos.y = m_pChar->GetCurObstacleLedge() + 0.5f;
		break;
	case YZ:
		obsPos.x = 0.0f;
		charPos.x = 0.0f;

		obsPos.y = m_pChar->GetCurObstacleLedge() + 0.5f;

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
	if (dist > m_properDistance)
		correctMovementDt *= _dt * m_deltaIntensity;

	m_pChar->GetController().lock()->AddMovementInput(correctMovementDt);
}
