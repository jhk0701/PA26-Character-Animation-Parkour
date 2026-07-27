#include "pch.h"
#include "CorrectRotation.h"
#include "Content/Character.h"
#include "Content/ContentConfig.h"
#include "Scene/IObstacle.h"
#include "Core/Log.h"

using namespace MiniEngine;
using namespace Content::Config;

namespace 
{
	Vector3 GetAxis(IObstacle* _pObs, ETagAxis _axis) 
	{
		switch (_axis)
		{
		case ETagAxis::X:
			return _pObs->GetTransform().Right();
		case ETagAxis::Y:
			return _pObs->GetTransform().Up();
		case ETagAxis::Z:
			return _pObs->GetTransform().Forward();
		}

		return Vector3(0.0f);
	}

	bool TryYawRotate(const Vector3& _inDir, Quaternion& _outRot) 
	{
		// yaw 방향에 대해서 유효성 확인
		if (_inDir.x * _inDir.x + _inDir.z * _inDir.z < 1e-10f)
			return false;

		// x,z 값이 유효 -> 각도 계산
		_outRot = Quaternion::CreateFromYawPitchRoll(atan2f(_inDir.x, _inDir.z), 0.0f, 0.0f);
		return true;
	}
}


void CorrectFixedRotation::OnStart(AnimNotifyParam& _param)
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

void CorrectFixedRotation::Activate(float _dt, AnimNotifyParam& _param)
{
	if (!m_pChar)
		return;

	// 자체 회전 반영
	m_elapsedTime += _dt;
	const float w = std::clamp(m_elapsedTime / GetDuration(), 0.0f, 1.0f);
	m_pChar->GetRoot()->localTransform.rotation = Quaternion::Slerp(m_startRotation, m_endRotation, w);
}


void CorrectRotationTowardObstacle::OnStart(AnimNotifyParam& _param)
{
	AnimNotifyState::OnStart(_param);

	if (!_param.m_pActor)
		return;

	m_pChar = dynamic_cast<Character*>(_param.m_pActor);
	m_elapsedTime = 0.0f;

	const Transform& TF = m_pChar->GetRoot()->localTransform;
	m_startRotation = TF.rotation;

	IObstacle* pObs = m_pChar->GetCurObstacleInfo().m_pObstacle;
	if (!pObs)
		return;

	assert(GetDuration() > 1e-4f);

	//uint8_t t = 0;
	//if (pObs->TryGetTag(TAG_ENV_DETAIL, t) && (ETagEnvDetail)t == ETagEnvDetail::Beam)
	//{
	//	// beam 지형물
	//	pObs->TryGetTag(TAG_SUB_INFO, t);
	//	Vector3 dir = GetAxis(pObs, (ETagAxis)t);
	//	dir = { -dir.z, dir.y, dir.x };

	//	const float DOT = dir.Dot(TF.Forward());
	//	Vector3 toward = DOT > 0.0f ? dir : dir;

	//	if (TryYawRotate(toward, m_endRotation) == false)
	//		m_endRotation = m_startRotation;
	//	
	//	return;
	//}

	// 일반 지형물 
	// 부딪힌 노멀 기준으로 정렬
	Vector3 nrm = -m_pChar->GetCurObstacleInfo().m_obstacleHitNrm;
	nrm.y = 0.0f;
	nrm.Normalize();

	MG_LOG_INFO("[CorrectRotationTowardObs] nrm : ({}, {}, {})", nrm.x, nrm.y, nrm.z);

	if (TryYawRotate(nrm, m_endRotation) == false)
		m_endRotation = m_startRotation;
}

void CorrectRotationTowardObstacle::Activate(float _dt, AnimNotifyParam& _param)
{
	if (!m_pChar)
		return;

	m_elapsedTime += _dt;

	const float w = std::clamp(m_elapsedTime / GetDuration(), 0.0f, 1.0f);
	m_pChar->GetRoot()->localTransform.rotation = Quaternion::Slerp(m_startRotation, m_endRotation, w);
}
