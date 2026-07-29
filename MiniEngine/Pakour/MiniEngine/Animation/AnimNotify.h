#pragma once
#include "Animation/IAnimNotify.h"

namespace MiniEngine 
{
	class AnimNotify : public IAnimNotify
	{
	public:
		AnimNotify() {};
		virtual ~AnimNotify() {}

		void Init() override;
		void Update(float _dt, AnimNotifyParam& _param) override;
		void SetTime(float _time) { m_timeToCall = _time; }
		virtual void Activate(AnimNotifyParam& _param) = 0;

	private:
		float m_timeToCall{ 0.0f };
		float m_playElapsed{ 0.0f };
		bool m_bIsCalled{ false };
	};

	class AnimNotifyState : public IAnimNotify 
	{
	public:
		AnimNotifyState() {}
		virtual ~AnimNotifyState() {}

		void Init() override;
		void Update(float _dt, AnimNotifyParam& _param) override;
		void SetTime(float _start, float _end) { m_start = _start; m_end = _end; }

		virtual void OnStart(AnimNotifyParam& _param) {};
		virtual void Activate(float _dt, AnimNotifyParam& _param)  = 0;
		virtual void OnEnd(AnimNotifyParam& _param) {};

		float GetDuration() const { return m_end - m_start; }
		float GetElapsed() const { return m_playElapsed; }

	private:
		bool m_bStartCalled{ false };
		bool m_bEndCalled{ false };

		float m_start{ 0.0f };
		float m_end{ 0.0f };
		float m_playElapsed{ 0.0f };
	};
}