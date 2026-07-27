#pragma once
#include "Content/CharacterState/RotateFixState.h"

class ProtrudeState : public RotateFixState
{
public:
	virtual void OnStart() override;
	virtual void OnEnd() override;
	virtual void Refresh() override;

	virtual void Tick(float _dt);

private:
	void ProcessMovement(float _dt);
	void AlignToNormal();
};