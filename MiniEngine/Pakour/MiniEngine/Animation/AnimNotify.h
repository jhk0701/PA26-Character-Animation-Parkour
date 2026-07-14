#pragma once
#include "Animation/IAnimNotify.h"
#include <functional>

namespace MiniEngine 
{
	class AnimNotify : public IAnimNotify
	{
	public:
		AnimNotify(float _time, std::function<void()>&& _event);

		void Init() override;
		void Update(float _dt) override;

	private:
		float m_timeToCall{ 0.0f };
		float m_playElapsed{ 0.0f };
		bool m_bIsCalled{ false };

		std::function<void()> m_event;
	};
}