#include "pch.h"
#include "Content/CharacterState/RotateFixState.h"
#include "Content/Character.h"
#include "Platform/Input.h"


bool RotateFixState::TryYawRotateToward(const Vector3& _inDir, Quaternion& _outRot)
{
	// yaw 방향에 대해서 유효성 확인
	if (_inDir.x * _inDir.x + _inDir.z * _inDir.z < 1e-10f)
		return false;

	// x,z 값이 유효 -> 각도 계산
	_outRot = Quaternion::CreateFromYawPitchRoll(atan2f(_inDir.x, _inDir.z), 0.0f, 0.0f);
	return true;
}
