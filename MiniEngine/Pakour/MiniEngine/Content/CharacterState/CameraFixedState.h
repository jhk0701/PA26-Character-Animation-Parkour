#pragma once
#include "Content/CharacterStateMachine.h"

// 일부 상태의 경우
// 카메라의 회전 방식이 아예 달리짐
// 기본 :
	// 마우스 x 델타가 캐릭터의 yaw 회전
	// 마우스 y 델타가 카메라 홀더의 pitch 회전
// 일부 상태는 카메라 홀더의 yaw 회전으로 사용
// 이 클래스는 그런 상태들을 위함
class CameraFixedState : public CharacterState
{
public:
	virtual void OnStart() override;
	virtual void OnEnd() override;

};