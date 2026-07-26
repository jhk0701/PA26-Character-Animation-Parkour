#pragma once
#include "Content/CharacterState/RotateFixState.h"
#include "Content/ContentConfig.h"

class Character;

class BeamState : public RotateFixState
{
public:
	virtual void OnStart() override;
	virtual void Refresh() override;
	virtual void OnEnd() override;
	virtual void Tick(float _dt) override;
	virtual void CheckState() override;
	virtual void ProcessPerceptionResult(const Character::PerceptedObstacleInfo& _info) override;

protected:
	void AlignByAxis();

	Content::Config::ETagAxis GetAxis() const { return m_curAxis; }
	Actor* GetCurObs() const { return m_pCurObs; }
	bool ObstacleIsBeamType(Actor* _pObs);
	
	void GetDirectionByAxis(Vector3& _outDir);

private:
	Content::Config::ETagAxis m_curAxis;
	Actor* m_pCurObs{ nullptr };
};

// Beam 지형에 올라탄 상태
// 움직임을 snap해줄 것
class BeamStandState : public BeamState
{
public:
	void Tick(float _dt) override;
	// void CheckState() override;
	void ProcessPerceptionResult(const Character::PerceptedObstacleInfo& _info) override;

private:
	void ProcessMovement(float _dt);
	bool CheckEnableToMove(std::shared_ptr<Character>& _pChar);
};

// Beam 지형에 올라탄 상태
// 움직임을 snap해줄 것
class BeamHangingState : public BeamState
{
public:
	void Tick(float _dt) override;
	void ProcessPerceptionResult(const Character::PerceptedObstacleInfo& _info) override;

private:
	void ProcessMovement(float _dt);
	bool CheckEnableToMove(std::shared_ptr<Character>& _pChar, const Vector2& _inputDir);
};