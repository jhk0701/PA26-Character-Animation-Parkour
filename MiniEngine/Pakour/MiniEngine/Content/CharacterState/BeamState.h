#pragma once
#include "Content/CharacterState/RotateFixState.h"
#include "Perception/Config/ObstacleConfig.h"

class BeamState : public RotateFixState
{
public:
	virtual ~BeamState() {};

	virtual void OnStart() override;
	virtual void Refresh() override;
	virtual void OnEnd() override;
	virtual void Tick(float _dt) override;
	virtual	void LateTick(float _dt) override;

protected:
	virtual void AlignByAxis() = 0;

	MiniEngine::ETagAxis GetAxis() const { return m_curAxis; }
	IObstacle* GetCurrentObstacle() const override;
	
	bool ObstacleIsBeamType(Actor* _pObs);
	void GetDirectionByAxis(Vector3& _outDir);
	void AdjustPositionToObstacleInfo();

private:
	MiniEngine::ETagAxis m_curAxis;
	IObstacle* m_pCurrentObstacle{ nullptr };
};

// Beam 지형에 올라탄 상태
// 움직임을 snap해줄 것
class BeamStandState : public BeamState
{
public:
	void Tick(float _dt) override;

protected:
	void AlignByAxis() override;
	bool IsAlignToAxis();

private:
	void ProcessMovement(float _dt);
	bool CheckEnableToMove(std::shared_ptr<Character>& _pChar);
	void AdjustRotationToObstacleInfo();
};

// Beam 지형에 올라탄 상태
// 움직임을 snap해줄 것
class BeamHangingState : public BeamState
{
public:
	void OnStart() override;
	void OnEnd() override;

	void Tick(float _dt) override;

protected:
	void AlignByAxis() override;

private:
	void ProcessMovement(float _dt);
	bool CheckEnableToMove(std::shared_ptr<Character>& _pChar, const Vector2& _inputDir);
};