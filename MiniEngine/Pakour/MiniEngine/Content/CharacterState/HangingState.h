#pragma once
#include "Content/CharacterState/RotateFixState.h"
#include "Content/ContentConfig.h"

class HangingState : public RotateFixState
{
public:
	virtual ~HangingState() {};
	
	virtual void OnStart() override;
	virtual void OnEnd() override;
	virtual void Tick(float _dt) override;
	virtual void LateTick(float _dt) override;
	void Refresh() override;

	IObstacle* GetCurrentObstacle() const override;

protected:
	virtual void AlignToNormal();
	void ClearCurObstacle() { m_pCurrentObstacle = nullptr; }

private:
	IObstacle* m_pCurrentObstacle;
};

class PoleHangingState : public HangingState 
{
public:
	void OnStart() override;
	void OnEnd() override;

protected:
	void AlignToNormal() override;

	void AlignDefault();
	void AlighAxis(uint8_t _axis);
};