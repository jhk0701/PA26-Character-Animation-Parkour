#pragma once
#include "Asset/AnimClip.h"
#include "Animation/IAnimNotify.h"
#include <functional>

namespace MiniEngine
{
	// 단발 재생용
	class ActionClip
	{
	public:
		ActionClip();
		~ActionClip() { m_clip = nullptr; }

		void Play();
		void Stop();

		void Sample(float _dt, const Skeleton& _skeleton, LocalPoseTRS& _outPose);
		float GetDuration() const { return m_duration; }
		AnimClip* GetClip() const { return m_clip; }
		const float GetTickPerSec() const;
		
		void AddClip(AnimClip* _clip);
		void AddNotify(std::shared_ptr<IAnimNotify> _notify);
		
		void SetApplyRootBone(bool _bEnable) { m_bApplyRootBone = _bEnable; }
		bool IsApplyingRootBone() const { return m_bApplyRootBone; }

	private:
		bool m_bIsPlaying{ false };
		bool m_bApplyRootBone{ true };

		float m_playTime{ 0.0f };
		float m_duration{ 0.0f };

		AnimClip* m_clip{ nullptr };
		std::vector<std::shared_ptr<IAnimNotify>> m_vecNotify;
	};
}