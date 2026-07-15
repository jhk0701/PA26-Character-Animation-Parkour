#include "pch.h"
#include "Content/EnableHangingState.h"
#include "Content/Character.h"

void EnableHangingState::Activate(MiniEngine::AnimNotifyParam& _param)
{
	if (!_param.m_pActor)
		return;

	Character* pChar = dynamic_cast<Character*>(_param.m_pActor);
	pChar->SetHangingState(m_bEnable);
}
