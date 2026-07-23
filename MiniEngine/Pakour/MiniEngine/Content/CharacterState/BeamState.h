#pragma once
#include "Content/CharacterState/CameraFixedState.h"
#include "Content/ContentConfig.h"

class Character;

class BeamState : public CameraFixedState 
{
public:
	virtual void OnStart() override;
	virtual void OnEnd() override;
	virtual void Tick(float _dt) override;
	virtual void CheckState() override;
	virtual void ProcessPerceptionResult(const Character::PerceptedObstacleInfo& _info) override;

protected:
	virtual void OrientByAxis() = 0;

	Content::Config::ETagAxis GetAxis() const { return m_curAxis; }
	Actor* GetCurObs() const { return m_pCurObs; }
	bool ObstacleIsBeamType(Actor* _pObs);
	
	Vector3 GetDirectionByAxis();

private:
	Content::Config::ETagAxis m_curAxis;
	Actor* m_pCurObs{ nullptr };
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

protected:
	void OrientByAxis() override;

private:
	void ProcessMovement(float _dt);
	bool IsAlignToAxis(std::shared_ptr<Character> _pChar);
	bool CheckEnableToMove();
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

protected:
	void OrientByAxis() override;

private:
	void ProcessMovement(float _dt);
};