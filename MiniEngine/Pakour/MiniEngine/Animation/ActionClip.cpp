#include "pch.h"
#include "Animation/ActionClip.h"

namespace MiniEngine 
{
	ActionClip::ActionClip()
	{
		// m_clips.reserve(4);
	}

	//ActonClip::ActonClip(int _reserveCnt)
	//{
	//	// m_clips.reserve(_reserveCnt);
	//}

	void ActionClip::Play()
	{
		m_bIsPlaying = true;
		m_playTime = 0.0f;
	}

	void ActionClip::Stop()
	{
		m_bIsPlaying = false;
		m_playTime = 0.0f;
	}

	void ActionClip::Sample(float _dt, const Skeleton& _skeleton, LocalPoseTRS& _outPose)
	{
		if (!m_bIsPlaying)
			return;

		if (!m_clip)
		{
			Stop();
			return;
		}

		m_playTime += _dt;
		if (m_playTime > m_duration)
		{
			// 재생 종료
			Stop();
			return;
		}

		// 포즈는 바로 적용할 것
		m_clip->SampleTRS(m_playTime, _skeleton, _outPose);
	}

	void ActionClip::AddClip(AnimClip* _clip)
	{
		// 애니메이션 duration : frame cnt 
		// 1s 기준으로 바꿔줘야함
		float s = 1.0f / ((_clip->ticksPerSecond > 0.0f) ? _clip->ticksPerSecond : 1.0f);
		m_duration += _clip->duration * s;
		m_clip = _clip;
	}
}