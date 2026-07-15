#include "pch.h"
#include "Content/JumpTiming.h"
#include "Content/Character.h"

void JumpTiming::Activate(MiniEngine::AnimNotifyParam& _param)
{
	if (!_param.m_pActor)
		return;

	Character* pChar = dynamic_cast<Character*>(_param.m_pActor);
	pChar->Jump();
}
