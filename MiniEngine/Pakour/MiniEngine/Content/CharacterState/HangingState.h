#pragma once
#include "Content/CharacterState/RotateFixState.h"
#include "Content/ContentConfig.h"

class HangingState : public RotateFixState
{
public:
	void OnStart() override;
	void OnEnd() override;
	void Tick(float _dt) override;
	void ProcessPerceptionResult(const Character::PerceptedObstacleInfo& _info) override;
	void Refresh() override;

private:
	void AlignToNormal();
	void ProcessMovement(float _dt);
	bool CheckEnableToMove(Content::Config::ETagAct _tag);

	// 인식 결과 처리. 어느 방향을 살폈는지는 입력에서 재유도한다(트리의 분기 기준과 동일)
	void OnPerceiveUp(const Character::PerceptedObstacleInfo& _info);
	void OnPerceiveDown(const Character::PerceptedObstacleInfo& _info);
	void OnPerceiveSide(const Character::PerceptedObstacleInfo& _info);
};