#include "pch.h"
#include "Animation/AnimNotify.h"

namespace MiniEngine 
{
	AnimNotify::AnimNotify(float _time, std::function<void()>&& _event) :
		m_timeToCall(_time), m_event(_event)
	{}

	void AnimNotify::Init() 
	{
		m_bIsCalled = false;
		m_playElapsed = 0.0f;
	}

	void AnimNotify::Update(float _dt) 
	{
		m_playElapsed += _dt;
		
		if (m_bIsCalled || m_playElapsed < m_timeToCall)
			return;

		m_bIsCalled = true;

		if (m_event)
			m_event();
	}
}