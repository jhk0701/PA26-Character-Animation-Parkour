#include "pch.h"
#include "Content/CorrectRootMotion.h"
#include "Content/Character.h"
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
	MiniEngine::Transform& tf = pCurObs->GetRoot()->localTransform;
	float distSq = MiniEngine::Vector3::DistanceSquared(m_pChar->GetRoot()->localTransform.position, tf.position);

	// 캐릭터와 장애물의 적정거리 보정
	if (distSq > m_properDistance * m_properDistance) 
	{
		// 
	}
	else 
	{

	}
}
