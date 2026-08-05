#pragma once
#include "Content/CharacterState/RotateFixState.h"
#include "Content/ContentConfig.h"

class HangingState : public RotateFixState
{
public:
	void OnStart() override;
	void OnEnd() override;
	void Tick(float _dt) override;
	void LateTick(float _dt) override;
	void Refresh() override;

private:
	void AlignToNormal();
	void ProcessMovement(float _dt);
	bool CheckEnableToMove(Content::Config::ETagAct _tag);
};