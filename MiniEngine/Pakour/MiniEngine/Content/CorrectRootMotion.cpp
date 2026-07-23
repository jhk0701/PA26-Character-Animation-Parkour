#include "pch.h"
#include "Content/CorrectRootMotion.h"
#include "Content/Character.h"
#include "Scene/CharacterControllerComponent.h"
#include "Core/Log.h"

using namespace MiniEngine;

// 사용 전제
// Perception Component를 통해서 지형을 식별하고
// 식별한 지형의 위치를 알고 있는 경우
void CorrectRootMotion::OnStart(AnimNotifyParam& _param)
{
	AnimNotifyState::OnStart(_param);

	if (!_param.m_pActor)
		return;

	m_pChar = dynamic_cast<Character*>(_param.m_pActor);
	m_elapsedTime = 0.0f;

	assert(GetDuration() > 1e-4f);
}

void CorrectRootMotion::Activate(float _dt, AnimNotifyParam& _param)
{
	if (!m_pChar)
		return;

	const Character::PerceptedObstacleInfo& OBS_INFO = m_pChar->GetCurObstacleInfo();
	const Transform& TF = m_pChar->GetRoot()->localTransform;

	// 캐릭터와 장애물의 적정거리 보정
	Vector3 obsPos = OBS_INFO.m_obstacleHitPos;
	obsPos.y = OBS_INFO.m_obstacleLedge;

	Vector3 charPos = TF.position;
	
	switch (m_corrextAxis)
	{
	case ECorrectAxis::XZ:
		obsPos.y = 0.0f;
		charPos.y = 0.0f;
		break;
	case ECorrectAxis::XY:
		obsPos.z = 0.0f;
		charPos.z = 0.0f;
		break;
	case ECorrectAxis::YZ:
		obsPos.x = 0.0f;
		charPos.x = 0.0f;
		break;
	case ECorrectAxis::None: __fallthrough;
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

	m_pChar->AddMovementInput(correctMovementDt);

	{
		return;
		// 정석적인 방법은 아래이나
		// 위의 처리 방식이 더 자연스러워 보여 우선 보류

		m_elapsedTime += _dt;
		float w = m_elapsedTime / GetDuration();
		Vector3 lerpedPos = Vector3::Lerp(charPos, properPoint, w);
		m_pChar->SetPosition(lerpedPos);
	}
}

// 사용 전제
// Perception Component를 통해서 지형을 식별하고
// 식별한 지형의 위치를 알고 있는 경우
void BezierCorrectRootMotion::OnStart(AnimNotifyParam& _param)
{
	AnimNotifyState::OnStart(_param);
	
	if (!_param.m_pActor)
		return;
	
	m_elapsedTime = 0.0f;

	// 시작 전, 캐릭터를 통해서 목표지점 이어받기
	m_pChar = dynamic_cast<Character*>(_param.m_pActor);
	if (m_pChar)
	{
		m_startPoint = m_pChar->GetRoot()->localTransform.position;

		const Character::PerceptedObstacleInfo& OBS_INFO = m_pChar->GetCurObstacleInfo();

		m_endPoint = OBS_INFO.m_obstacleHitPos;
		m_endPoint.y = OBS_INFO.m_obstacleLedge;
		m_endPoint += m_endOffset;

		m_midPoint = Vector3::Lerp(m_startPoint, m_endPoint, 0.5f);
		m_midPoint.y += m_bezierY;
	}

	assert(GetDuration() > 1e-4f);
}

void BezierCorrectRootMotion::Activate(float _dt, AnimNotifyParam& _param)
{
	if (!m_pChar)
		return;

	// 베지어로 보간
	m_elapsedTime += _dt;
	const float w = m_elapsedTime / GetDuration();
	Vector3 p1 = Vector3::Lerp(m_startPoint, m_midPoint, w);
	Vector3 p2 = Vector3::Lerp(m_midPoint, m_endPoint, w);
	Vector3 p3 = Vector3::Lerp(p1, p2, w);
	
	m_pChar->SetPosition(p3);
}

void RotateMotion::OnStart(MiniEngine::AnimNotifyParam& _param)
{
	AnimNotifyState::OnStart(_param);

	if (!_param.m_pActor)
		return;

	m_pChar = dynamic_cast<Character*>(_param.m_pActor);
	m_elapsedTime = 0.0f;

	m_startRotation = m_pChar->GetRoot()->localTransform.rotation;
	m_endRotation = m_startRotation * Quaternion::CreateFromYawPitchRoll(ToRadians(m_rotateDegree.x), ToRadians(m_rotateDegree.y), ToRadians(m_rotateDegree.z));
	m_endRotation.Normalize();

	assert(GetDuration() > 1e-4f);
}

void RotateMotion::Activate(float _dt, MiniEngine::AnimNotifyParam& _param)
{
	if (!m_pChar)
		return;

	// 자체 회전 반영
	m_elapsedTime += _dt;

	const float w = m_elapsedTime / GetDuration();
	m_pChar->GetRoot()->localTransform.rotation = Quaternion::Slerp(m_startRotation, m_endRotation, w);
}
