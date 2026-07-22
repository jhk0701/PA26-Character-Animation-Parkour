#include "pch.h"
#include "Content/UseGravity.h"
#include "Content/Character.h"

void UseGravity::Activate(MiniEngine::AnimNotifyParam& _param)
{
	if (!_param.m_pActor)
		return;

	Character* pChar = dynamic_cast<Character*>(_param.m_pActor);
	pChar->SetUseGravity(m_bUseGravity);
}
