#include "pch.h"
#include "Content/AnimNotify/CharacterIKNotify.h"
#include "Content/Character.h"

#include "Core/Log.h"

void CharacterIKEnabler::OnStart(MiniEngine::AnimNotifyParam& _param)
{
	AnimNotifyState::OnStart(_param);

	m_pChar = dynamic_cast<Character*>(_param.m_pActor);
	if (!m_pChar)
		return;

	for (const uint8_t& t : m_ikTypes)
		m_pChar->SetIKAlpha(t, m_from);
	
	m_elapsedTime = 0.0f;
	assert(GetDuration() > 1e-4f);
}

void CharacterIKEnabler::OnEnd(MiniEngine::AnimNotifyParam& _param)
{
	AnimNotifyState::OnEnd(_param);

	for (const uint8_t& t : m_ikTypes)
	{
		m_pChar->SetIKAlpha(t, m_to);
		// MG_LOG_INFO("[CharacterIKEnabler::OnEnd] Set Last Value {}, {:.3f}", t, m_to);
	}
}

void CharacterIKEnabler::Activate(float _dt, MiniEngine::AnimNotifyParam& _param)
{
	if (!m_pChar)
		return;

	m_elapsedTime += _dt;
	const float W = m_elapsedTime / GetDuration();
	const float LERP = m_from + (m_to - m_from) * W;

	for (const uint8_t& t : m_ikTypes)
		m_pChar->SetIKAlpha(t, LERP);
}

// 현재 캐릭터가 파쿠르 중이라면
// 파구르 중인 장애물에 손이나 발을 짚는 IK 실행
void CharacterIKInvoker::OnStart(MiniEngine::AnimNotifyParam& _param)
{
	AnimNotifyState::OnStart(_param);

	m_pChar = dynamic_cast<Character*>(_param.m_pActor);
	assert(GetDuration() > 1e-4f);
}

void CharacterIKInvoker::Activate(float _dt, MiniEngine::AnimNotifyParam& _param)
{
	if (!m_pChar)
		return;

	const Transform& TF = m_pChar->GetRoot()->localTransform;
	Vector3 offset(0.0f);
	offset += TF.Right() * m_posOffset.x;
	offset += TF.Up() * m_posOffset.y;
	offset += TF.Forward() * m_posOffset.z;

	m_pChar->IKDetectObstacle(m_ikType, offset);
}

void CharacterIKInvokerFixedPoint::OnStart(MiniEngine::AnimNotifyParam& _param)
{
	AnimNotifyState::OnStart(_param);

	m_pChar = dynamic_cast<Character*>(_param.m_pActor);
	assert(GetDuration() > 1e-4f);
}

void CharacterIKInvokerFixedPoint::Activate(float _dt, MiniEngine::AnimNotifyParam& _param)
{
	if (!m_pChar)
		return;

	const Transform& TF = m_pChar->GetRoot()->localTransform;
	Vector3 offset(0.0f);
	offset += TF.Right() * m_posOffset.x;
	offset += TF.Up() * m_posOffset.y;
	offset += TF.Forward() * m_posOffset.z;

	m_pChar->IKSetFixedPoint(m_ikType, offset);
}
