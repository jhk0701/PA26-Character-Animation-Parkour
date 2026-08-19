#include "pch.h"
#include "Content/UseGravity.h"
#include "Content/Character.h"
#include "Scene/CharacterControllerComponent.h"

void UseGravityNotifyState::OnStart(MiniEngine::AnimNotifyParam& _param)
{
	AnimNotifyState::OnStart(_param);

	if (Character* pChar = dynamic_cast<Character*>(_param.m_pActor)) 
	{
		pChar->GetController().lock()->SetUseGravityForRootMotion(true);
	}
}

void UseGravityNotifyState::OnEnd(MiniEngine::AnimNotifyParam& _param)
{
	AnimNotifyState::OnEnd(_param);

	if (Character* pChar = dynamic_cast<Character*>(_param.m_pActor))
	{
		pChar->GetController().lock()->SetUseGravityForRootMotion(false);
	}
}
