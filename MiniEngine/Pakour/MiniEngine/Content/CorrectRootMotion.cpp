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

	m_elapsedTime = 0.0f;
	m_pChar = dynamic_cast<Character*>(_param.m_pActor);

	if (m_pChar)
		m_startPos = m_pChar->GetRoot()->localTransform.position;

	assert(GetDuration() > 1e-4f);
}

void CorrectRootMotion::Activate(float _dt, AnimNotifyParam& _param)
{
	if (!m_pChar)
		return;

	const PerceptedObstacleInfo& OBS_INFO = m_pChar->GetCurObstacleInfo();

	// 캐릭터와 장애물의 적정거리 보정
	Vector3 obsPos = OBS_INFO.perceptResult.obstacleHitPos;
	obsPos.y = OBS_INFO.perceptResult.obstacleLedge;

	Vector3 charPos = m_startPos;
	
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

	m_elapsedTime += _dt;
	const float w = m_elapsedTime / GetDuration();
	Vector3 lerpedPos = Vector3::Lerp(charPos, properPoint, w);
	switch (m_corrextAxis)
	{
	case ECorrectAxis::XZ:
		lerpedPos.y = m_startPos.y;
		break;
	case ECorrectAxis::XY:
		lerpedPos.z = m_startPos.z;
		break;
	case ECorrectAxis::YZ:
		lerpedPos.x = m_startPos.x;
		break;
	case ECorrectAxis::None: __fallthrough;
	default:
		break;
	}

	m_pChar->SetPosition(lerpedPos);
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
	if (!m_pChar)
		return;

	const Transform& TF = m_pChar->GetRoot()->localTransform;
	m_startPoint = TF.position;

	const PerceptedObstacleInfo& OBS_INFO = m_pChar->GetCurObstacleInfo();

	m_endPoint = OBS_INFO.perceptResult.obstacleHitPos;
	m_endPoint.y = OBS_INFO.perceptResult.obstacleLedge;

	Vector3 endOffset(0.0f);
	endOffset += TF.Right() * m_endOffset.x;
	endOffset += TF.Up() * m_endOffset.y;
	endOffset += TF.Forward() * m_endOffset.z;
	m_endPoint += endOffset;
	m_midPoint = Vector3::Lerp(m_startPoint, m_endPoint, 0.5f);
	
	Vector3 midOffset(0.0f);
	midOffset += TF.Right() * m_midOffset.x;
	midOffset += TF.Up() * m_midOffset.y;
	midOffset += TF.Forward() * m_midOffset.z;
	m_midPoint += midOffset;

	// 마스크 적용
	if (m_bAxisMask[0] == false)
	{
		m_midPoint.x = m_startPoint.x;
		m_endPoint.x = m_startPoint.x;
	}

	if (m_bAxisMask[1] == false) 
	{
		m_midPoint.y = m_startPoint.y;
		m_endPoint.y = m_startPoint.y;
	}

	if (m_bAxisMask[2] == false)
	{
		m_midPoint.z = m_startPoint.z;
		m_endPoint.z = m_startPoint.z;
	}

	assert(GetDuration() > 1e-4f);
}

void BezierCorrectRootMotion::OnEnd(MiniEngine::AnimNotifyParam& _param)
{
	AnimNotifyState::OnEnd(_param);

	/*
	const Vector3 POS = m_pChar->GetRoot()->localTransform.position;
	MG_LOG_INFO("[Bezier Correction] End :: charPos : ({:.2f}, {:.2f}, {:.2f})", POS.x, POS.y, POS.z);
	*/
}

void BezierCorrectRootMotion::Activate(float _dt, AnimNotifyParam& _param)
{
	if (!m_pChar)
		return;

	// 베지어로 보간
	m_elapsedTime += _dt;
	const float w = std::clamp(m_elapsedTime / GetDuration(), 0.0f, 1.0f);

	Vector3 p1 = Vector3::Lerp(m_startPoint, m_midPoint, w);
	Vector3 p2 = Vector3::Lerp(m_midPoint, m_endPoint, w);
	Vector3 p3 = Vector3::Lerp(p1, p2, w);

	// MG_LOG_INFO("[Bezier Correct] w : {:.2f},  pos : ({:.2f}, {:.2f}, {:.2f})", w, p3.x, p3.y, p3.z);
	m_pChar->SetPosition(p3);
}
