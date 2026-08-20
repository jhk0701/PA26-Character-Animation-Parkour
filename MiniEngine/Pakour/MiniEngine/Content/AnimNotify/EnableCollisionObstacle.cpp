#include "pch.h"
#include "Content/AnimNotify/EnableCollisionObstacle.h"
#include "Content/Character.h"

void EnableCollisionObstacle::Activate(MiniEngine::AnimNotifyParam& _param)
{
	if (!_param.m_pActor)
		return;

	Character* pChar = dynamic_cast<Character*>(_param.m_pActor);
	pChar->SetEnableCollisionObstacle(m_bEnable);
};