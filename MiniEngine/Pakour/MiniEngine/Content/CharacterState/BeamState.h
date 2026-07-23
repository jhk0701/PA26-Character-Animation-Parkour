#pragma once
#include "Content/CharacterState/CameraFixedState.h"
#include "Content/ContentConfig.h"

class BeamState : public CameraFixedState 
{
public:
	virtual void OnStart() override;
	virtual void OnEnd() override;
	virtual void Tick(float _dt) override;
	virtual void CheckState() override;
	virtual void ProcessPerceptionResult(const Character::PerceptedObstacleInfo& _info) override;

protected:
	Content::Config::ETagAxis GetAxis() const { return m_curAxis; }

private:
	Content::Config::ETagAxis m_curAxis;
};

// Beam 지형에 올라탄 상태
// 움직임을 snap해줄 것
class BeamStandState : public BeamState
{
public:
	void OnStart() override;
	void OnEnd() override;
	void Tick(float _dt) override;
	void CheckState() override;
	void ProcessPerceptionResult(const Character::PerceptedObstacleInfo& _info) override;

private:
	void ProcessMovement(float _dt);
};

// Beam 지형에 올라탄 상태
// 움직임을 snap해줄 것
class BeamHangingState : public BeamState
{
public:
	void OnStart() override;
	void OnEnd() override;
	void Tick(float _dt) override;
	void CheckState() override;
	void ProcessPerceptionResult(const Character::PerceptedObstacleInfo& _info) override;

private:
	void ProcessMovement(float _dt);
};