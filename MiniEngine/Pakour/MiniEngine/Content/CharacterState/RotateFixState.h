#pragma once
#include "Content/CharacterStateMachine.h"

class RotateFixState : public CharacterState
{
protected:
	bool TryYawRotateToward(const Vector3& _inDir, Quaternion& _outRot);
};