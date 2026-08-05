#include "pch.h"
#include "Content/AddMovementNotifyState.h"
#include "Content/Character.h"

void AddMovementNotifyState::OnStart(AnimNotifyParam& _param)
{
	AnimNotifyState::OnStart(_param);

	m_pChar = dynamic_cast<Character*>(_param.m_pActor);
}

void AddMovementNotifyState::Activate(float _dt, AnimNotifyParam& _param)
{
	if (!m_pChar)
		return;

	const Transform& TF = m_pChar->GetRoot()->localTransform;

	Vector3 relativeDir(0.0f);
	relativeDir += TF.Right() * m_dir.x;
	relativeDir += TF.Up() * m_dir.y;
	relativeDir += TF.Forward() * m_dir.z;
	
	relativeDir.Normalize();

	m_pChar->AddMovementInput(_dt * m_pChar->GetMoveSpeed() * relativeDir);
}