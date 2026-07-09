#pragma once
#include "Asset/AnimClip.h"

namespace MiniEngine 
{
	struct Transform;
	class IAnimatorClip 
	{
	public:
		virtual ~IAnimatorClip() {};
		
		// 애니메이션 샘플링 메서드
		// 루트모션 처리용으로 오너 액터의 root transform 전달
		virtual void Sample(float _dt, const Skeleton& _skeleton, LocalPoseTRS& _outPose, Transform& _rootTrs) = 0;
		virtual float GetDuration() const = 0;
	};
}