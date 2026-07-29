#include "pch.h"
#include "Content/CharacterIKNotify.h"
#include "Character.h"

#include "Core/Log.h"

void CharacterIKEnabler::OnStart(MiniEngine::AnimNotifyParam& _param)
{
	AnimNotifyState::OnStart(_param);

	m_pChar = dynamic_cast<Character*>(_param.m_pActor);
	if (!m_pChar)
		return;

	m_pChar->SetIKAlpha(m_ikType, m_from);
	m_elapsedTime = 0.0f;

	assert(GetDuration() > 1e-4f);
	assert(m_from != m_to);
}

void CharacterIKEnabler::Activate(float _dt, MiniEngine::AnimNotifyParam& _param)
{
	if (!m_pChar)
		return;

	m_elapsedTime += _dt;
	const float W = m_elapsedTime / GetDuration();
	const float LERP = m_from + (m_to - m_from) * W;
	m_pChar->SetIKAlpha(m_ikType, LERP);
}

void CharacterIKEnabler::OnEnd(MiniEngine::AnimNotifyParam& _param)
{
	AnimNotifyState::OnEnd(_param);

	m_pChar->SetIKAlpha(m_ikType, m_to);
}


// 전제 조건.
// 캐릭터가 파쿠르 중일 것
// 캐릭터로부터 현재 파쿠르 중인 정보를 받아와야함
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
