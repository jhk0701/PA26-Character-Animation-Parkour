#include "pch.h"
#include "Content/CorrectRootMotion.h"
#include "Content/Character.h"

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

	// 현재 장애물과 거리

	// 캐릭터와 장애물의 적정거리 보정
	
}
