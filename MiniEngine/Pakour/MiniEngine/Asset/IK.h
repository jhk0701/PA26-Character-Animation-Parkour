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

	// 관절 한계. 기본값은 전부 "무제한" 이라 넣지 않으면 기존 동작 그대로다.
	//
	// 굽힘각(bend)은 lower 관절의 내각이다 — 0도 = 완전히 접힘, 180도 = 완전히 펴짐.
	// 코사인 법칙 d^2 = u^2 + l^2 - 2*u*l*cos(B) 가 굽힘각과 사거리를 일대일(단조증가)로
	// 대응시키므로, 각도 한계는 그대로 사거리 한계로 접혀 ClampReach 안에서 처리된다.
	//
	// ⚠ maxBendDeg 는 기존 상한(0.995 * (u+l))보다 낮아야 효과가 있다. 다리(u≈l≈0.42m)
	//   기준 그 상한이 이미 B ≈ 168.6도 라, 175 같은 값은 조용히 무효다.
	struct TwoBoneIKLimits
	{
		float minBendDeg{ 0.0f };     // 접힘 한계.   0   = 무제한
		float maxBendDeg{ 180.0f };   // 신전 한계. 180   = 무제한
		float swingConeDeg{ 180.0f }; // upper 가 coneAxis 에서 벌어질 최대각. 180 = 무제한

		// 콘의 중심축. 솔버가 쓰는 좌표계(= 본 위치들과 같은 공간, 이 엔진에선 메시 로컬)의
		// 단위벡터여야 한다. swingConeDeg < 180 일 때만 읽는다.
		// 커널은 스켈레톤을 모르므로 축 유도는 호출자 몫이다 (Animator::BindSwingAxis).
		Vector3 coneAxis;
	};

	struct TwoBoneIKTarget
	{
		Vector3 targetPos;
		Vector3 poleTargetPos;
		float alpha;

		TwoBoneIKLimits limits{};
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