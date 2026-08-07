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

	IObstacle* GetCurrentObstacle() const override;


private:
	void AlignToNormal();

	IObstacle* m_pCurrentObstacle;
};