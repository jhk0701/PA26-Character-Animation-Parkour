#pragma once
#include "Content/CharacterState/RotateFixState.h"
#include "Content/ContentConfig.h"

class Character;
namespace MiniEngine 
{
	class IObstacle;
}

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

	Content::Config::ETagAxis GetAxis() const { return m_curAxis; }
	MiniEngine::IObstacle* GetCurObs() const { return m_pCurObs; }
	bool ObstacleIsBeamType(Actor* _pObs);
	void GetDirectionByAxis(Vector3& _outDir);
	void AdjustPositionToObstacleInfo();

private:
	Content::Config::ETagAxis m_curAxis;
	MiniEngine::IObstacle* m_pCurObs{ nullptr };
};

// Beam 지형에 올라탄 상태
// 움직임을 snap해줄 것
class BeamStandState : public BeamState
{
public:
	void Tick(float _dt) override;
	void ProcessPerceptionResult(const Character::PerceptedObstacleInfo& _info) override;

protected:
	void AlignByAxis() override;
	bool IsAlignToAxis();

private:
	void ProcessMovement(float _dt);
	bool CheckEnableToMove(std::shared_ptr<Character>& _pChar);
};

// Beam 지형에 올라탄 상태
// 움직임을 snap해줄 것
class BeamHangingState : public BeamState
{
public:
	void OnStart() override;
	void OnEnd() override;

	void Tick(float _dt) override;
	void ProcessPerceptionResult(const Character::PerceptedObstacleInfo& _info)  override;

protected:
	void AlignByAxis() override;

private:
	void ProcessMovement(float _dt);
	bool CheckEnableToMove(std::shared_ptr<Character>& _pChar, const Vector2& _inputDir);
};