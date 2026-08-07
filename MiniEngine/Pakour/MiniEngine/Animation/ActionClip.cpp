#include "pch.h"
#include "Animation/ActionClip.h"

namespace MiniEngine 
{
	ActionClip::ActionClip() { }

	void ActionClip::Play()
	{
		m_bIsPlaying = true;
		m_playTime = GetStartTime();

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

		const float MUL_SPD_DT = _dt * m_speed;
		m_playTime += MUL_SPD_DT;

		bool bIsLast = false;
		const float END_TIME = GetEndTime();

		if (m_playTime >= END_TIME)
		{
			m_playTime = END_TIME;
			bIsLast = true;
		}

		// 포즈는 바로 적용할 것
		m_clip->SampleTRS(m_playTime, _skeleton, _outPose);
		
		for (const std::shared_ptr<IAnimNotify>& n : m_vecNotify)
			n->Update(MUL_SPD_DT, _notifyParam);

		if (bIsLast)
		{
			// 재생 종료
			Stop();
			return;
		}
	}

	const float ActionClip::GetDuration() const
	{
		float cliped = m_duration;
		cliped -= (m_startOffset + m_endOffset);

		return cliped;
	}

	const float ActionClip::GetStartTime() const { return m_startOffset; }
	const float ActionClip::GetEndTime() const { return m_duration - m_endOffset; }

	void ActionClip::AddClip(AnimClip* _clip)
	{
		m_duration += _clip->ClipDurationSec();
		m_clip = _clip;
	}

	const float ActionClip::GetTickPerSec() const { return m_clip->ticksPerSecond; }
	void ActionClip::AddNotify(std::shared_ptr<IAnimNotify> _notify) { m_vecNotify.push_back(_notify); }

	void ActionClip::SetSpeed(const float _spd)
	{
		// 유효성 검사
		assert(_spd > 1e-4f && _spd > 0.0f);
		m_speed = _spd;
	}

	void ActionClip::SetOffset(const float _startOffset, const float _endOffset)
	{
		// 유효성 검사
		assert(m_duration > _startOffset + _endOffset);

		m_startOffset = _startOffset;
		m_endOffset = _endOffset;
	}

}