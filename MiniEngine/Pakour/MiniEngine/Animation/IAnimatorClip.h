#pragma once
#include "Asset/AnimClip.h"

namespace MiniEngine 
{
	class IAnimatorClip 
	{
	public:
		virtual ~IAnimatorClip() {};
		
		virtual void Sample(float _dt, const Skeleton& _skeleton, LocalPoseTRS& _outPose) = 0;
		virtual float GetDuration() const = 0;
	};
}