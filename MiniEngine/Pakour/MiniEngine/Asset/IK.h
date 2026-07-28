#pragma once
#include "Core/Math.h"

namespace MiniEngine 
{
	struct TwoBoneIKBone
	{
		Vector3 upperPos;
		Vector3 lowerPos;
		Vector3 endPos;
	};

	struct TwoBoneIKTarget 
	{
		Vector3 targetPos;
		Vector3 poleTargetPos;
		float alpha;
	};

	struct TwoBoneIKResult
	{
		Quaternion upperDelta;
		Quaternion lowerDelta;
	};

	
	Quaternion FromToRotation(const Vector3& _from, const Vector3& _to);

	// 현재 포즈의 굽힘 평면을 보존하는 폴 타깃(= 현재 중간 관절 위치)
	// 사지가 완전히 펴져 굽힘 평면이 정의되지 않으면 false — 다른 폴을 대야 함
	bool FallbackPole(const TwoBoneIKBone& _inBone, Vector3& _outPole);

	// 2본 체인 솔버 함수
	// upper -> lower -> end가 타겟 위치에 닿도록하는 델타 회전값 2개 반환
	bool SolveTwoBone(const TwoBoneIKBone& _inBone, const TwoBoneIKTarget& _inTarget, TwoBoneIKResult& _outResult);
}