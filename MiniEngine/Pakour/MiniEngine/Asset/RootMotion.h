#pragma once
#include "Core/Math.h"
#include "AnimClip.h"
#include "SkinnedMesh.h"

namespace MiniEngine
{
	// 루트모션 처리 방법
	// 1. 애니메이션 실행
	// 2. 실행된 포즈에서 루트에 해당하는 본의 Transform 델타값 추출
	// 3. 애니메이션 -> In Place 포즈 상태
	// 4. 제거한 델타값을 CPU에서 직접 localTransform에 적용

	// 루트모션에서 추출할 델타값
	struct RootMotionDelta
	{
		Vector3 translation = Vector3(0.0f, 0.0f, 0.0f);
		Quaternion rotation = Quaternion(0.0f, 0.0f, 0.0f, 1.0f);
		void Reset() 
		{
			translation = Vector3(0.0f);
			rotation = Quaternion(0.0f, 0.0f, 0.0f, 1.0f);
		}
	};

	// 추출 대상
	struct RootMotionConfig 
	{
		// x,z 축에 해당하는 위치 값은 기본적으로 추출
		bool extractY{ false };
		bool extractYaw{ true };
	};

	Quaternion ExtractYaw(const Quaternion& _q); // 쿼터니언에서 yaw 회전각 추출

	// 클립에서 루트모션 델타값 추출
	void ExtractClipRootMotion(const AnimClip& _clip, const Skeleton& _skel, float _t0, float _t1, RootMotionDelta& _outDelta, int _rootBone = 0);

	// 설정한 추출 대상에 따라 마스킹
	void ApplyRootMotionMask(const RootMotionConfig& _config, RootMotionDelta& _inout);

	// 애니메이션 샘플링이 끝난 후, 루트에서 모션 델타값 추출
	void StripRootMotionFromPose(const Skeleton& _skel, const RootMotionConfig& _config, LocalPoseTRS& _inoutPose, int _rootBone = 0);


	// 추출한 델타값 간 블렌드
	void BlendRootMotion(const RootMotionDelta& _a, const RootMotionDelta& _b, float _t, RootMotionDelta& _out);
}