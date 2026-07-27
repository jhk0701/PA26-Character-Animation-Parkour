#include "pch.h"
#include "Animation/ActionClip.h"

namespace MiniEngine 
{
	ActionClip::ActionClip()
	{
	}

	void ActionClip::Play()
	{
		m_bIsPlaying = true;
		m_playTime = 0.0f;

		for (const std::shared_ptr<IAnimNotify>& n : m_vecNotify)
			n->Init();
	}

	void ActionClip::Stop()
	{
		m_bIsPlaying = false;
		m_playTime = 0.0f;
	}

	void ActionClip::Sample(float _dt, const Skeleton& _skeleton, LocalPoseTRS& _outPose, AnimNotifyParam& _notifyParam)
	{
		if (!m_bIsPlaying)
			return;

		if (!m_clip)
		{
			Stop();
			return;
		}

		m_playTime += _dt;
		bool bIsLast = false;

		if (m_playTime >= m_duration)
		{
			m_playTime = m_duration;
			bIsLast = true;
		}

		// 포즈는 바로 적용할 것
		m_clip->SampleTRS(m_playTime, _skeleton, _outPose);
		
		for (const std::shared_ptr<IAnimNotify>& n : m_vecNotify)
			n->Update(_dt, _notifyParam);

		if (bIsLast)
		{
			// 재생 종료
			Stop();
			return;
		}
	}

	const float ActionClip::GetTickPerSec() const
	{
		return m_clip->ticksPerSecond;
	}

	void ActionClip::AddClip(AnimClip* _clip)
	{
		m_duration += _clip->ClipDurationSec();
		m_clip = _clip;
	}

	void ActionClip::AddNotify(std::shared_ptr<IAnimNotify> _notify)
	{
		m_vecNotify.push_back(_notify);
	}
}