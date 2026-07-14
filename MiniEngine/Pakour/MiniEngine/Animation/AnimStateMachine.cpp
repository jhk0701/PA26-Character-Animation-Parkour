#include "pch.h"
#include "Animation/AnimStateMachine.h"
#include "Animation/BlendClip.h"

namespace MiniEngine 
{
	void AnimStateMachine::Init(int _startIdx)
	{
		m_curStateIdx = _startIdx;
		m_prevIdx = 0;

		m_bIsInitialized = true;
	}

	void AnimStateMachine::Transition(int _newIdx, float _duration)
	{
		m_fadeElapsed = 0.0f;
		m_fadeDuration = _duration;
		m_prevIdx = m_curStateIdx;
		m_curStateIdx = _newIdx;
	}

	void AnimStateMachine::Update(float _dt, const Skeleton& _skeleton, LocalPoseTRS& _outPose)
	{
		if (m_curStateIdx < 0 || m_curStateIdx >= m_states.size() || m_bIsInitialized == false)
			return;

		if (IsFading() == false)
		{
			m_states[m_curStateIdx]->SetAxisValue(m_InputAxis);
			m_states[m_curStateIdx]->Sample(_dt, _skeleton, _outPose); // 포즈 타겟에 해당 index 바로 샘플링
			return;
		}

		m_fadeElapsed += _dt;

		m_states[m_curStateIdx]->SetAxisValue(m_InputAxis);
		m_states[m_prevIdx]->SetAxisValue(m_InputAxis); // 이전 포즈
		m_states[m_curStateIdx]->Sample(_dt, _skeleton, m_poseNext); // 메인 포즈
		m_states[m_prevIdx]->Sample(_dt, _skeleton, m_posePrev); // 이전 포즈

		const float w = std::clamp(m_fadeElapsed / m_fadeDuration, 0.0f, 1.0f);
		BlendPose(m_posePrev, m_poseNext, w, _outPose);

		if (m_fadeElapsed >= m_fadeDuration)
			m_fadeDuration = 0.0f; // 완료 시 초기화
	}
}
