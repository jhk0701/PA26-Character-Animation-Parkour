#pragma once
#include "Content/CharacterStateMachine.h"

class RotateFixState : public CharacterState
{
public:
	virtual void OnStart() override;
	virtual void OnEnd() override;

protected:
	bool TryYawRotateToward(const Vector3& _inDir, Quaternion& _outRot);
};