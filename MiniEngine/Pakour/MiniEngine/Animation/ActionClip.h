#pragma once
#include "Asset/AnimClip.h"

namespace MiniEngine
{
	// 단발 재생용
	class ActionClip
	{
	public:
		ActionClip();
		// ActonClip(int _reserveCnt);
		~ActionClip() { m_clip = nullptr; }

		void Play();
		void Stop();

		void Sample(float _dt, const Skeleton& _skeleton, LocalPoseTRS& _outPose);
		float GetDuration() const { return m_duration; }
		AnimClip* GetClip() const { return m_clip; }

		const float GetTickPerSec() const;

		void AddClip(AnimClip* _clip);

	private:
		bool m_bIsPlaying{ false };
		float m_playTime{ 0.0f };
		float m_duration{ 0.0f };

		AnimClip* m_clip{ nullptr };
		// std::vector<AnimClip*> m_clips;
	};
}