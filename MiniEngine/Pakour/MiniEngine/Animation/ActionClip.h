#pragma once
#include "Animation/IAnimatorClip.h"

namespace MiniEngine
{
	// 단발 재생용
	class ActionClip : public IAnimatorClip
	{
	public:
		ActionClip();
		// ActonClip(int _reserveCnt);
		~ActionClip() { m_clip = nullptr; }

		void Play();
		void Stop();

		void Sample(float _dt, const Skeleton& _skeleton, LocalPoseTRS& _outPose) override;
		float GetDuration() const override { return m_duration; }

		void AddClip(AnimClip* _clip);

	private:
		bool m_bIsPlaying{ false };
		float m_playTime{ 0.0f };
		float m_duration{ 0.0f };

		AnimClip* m_clip{ nullptr };
		// std::vector<AnimClip*> m_clips;
	};
}