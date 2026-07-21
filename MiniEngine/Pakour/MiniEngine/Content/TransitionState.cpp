#include "pch.h"
#include "Content/TransitionState.h"
#include "Content/Character.h"

void TransitionState::Activate(MiniEngine::AnimNotifyParam& _param)
{
	if (!_param.m_pActor)
		return;

	Character* pChar = dynamic_cast<Character*>(_param.m_pActor);
	pChar->TransitionStateMachine(m_targetState);
}