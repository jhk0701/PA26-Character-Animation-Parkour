#include "pch.h"
#include "Animation/AnimNotify.h"

namespace MiniEngine 
{
	void AnimNotify::Init() 
	{
		m_bIsCalled = false;
		m_playElapsed = 0.0f;
	}

	void AnimNotify::Update(float _dt, AnimNotifyParam& _param)
	{
		m_playElapsed += _dt;
		
		if (m_bIsCalled || m_playElapsed < m_timeToCall)
			return;

		m_bIsCalled = true;

		Activate(_param);
	}

	void AnimNotifyState::Init()
	{
		m_bStartCalled = false;
		m_bEndCalled = false;
		m_playElapsed = 0.0f;
	}

	void AnimNotifyState::Update(float _dt, AnimNotifyParam& _param)
	{
		m_playElapsed += _dt;

		if (m_playElapsed < m_start || m_playElapsed > m_end)
			return;

		if (!m_bStartCalled && m_playElapsed >= m_start)
		{
			m_bStartCalled = true;
			OnStart(_param);
		}

		Activate(_dt, _param);

		if (!m_bEndCalled && m_playElapsed >= m_end)
		{
			m_bEndCalled = true;
			OnEnd(_param);
		}
	}
}