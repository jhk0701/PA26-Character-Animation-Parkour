#pragma once
#include "Animation/AnimNotify.h"

using namespace MiniEngine;

class Character;
class AddMovementNotify : public AnimNotify
{
public:
	void Activate(AnimNotifyParam& _param) override;
	void SetDirection(const Vector3& _dir) { m_dir = _dir; }

private:
	Vector3 m_dir;
};

class AddMovementNotifyState : public AnimNotifyState
{
public:
	void OnStart(AnimNotifyParam& _param) override;
	void Activate(float _dt, AnimNotifyParam& _param) override;

	void SetDirection(const Vector3& _dir) { m_dir = _dir; }

private:
	Character* m_pChar;
	Vector3 m_dir;
};