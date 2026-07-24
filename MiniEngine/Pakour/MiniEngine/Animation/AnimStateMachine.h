#pragma once
#include "Asset/AnimClip.h"

namespace MiniEngine 
{
	class BlendClip;

	// 다수의 클립들을 연결 및 제어하는 FSM
	// 클립 간 트랜지션 필요
	class AnimStateMachine
	{
	public:
		AnimStateMachine() {};
		~AnimStateMachine() { m_states.clear(); }

		void Reserve(uint8_t _cnt) { m_states.reserve(_cnt); }
		void AddState(std::shared_ptr<BlendClip> _clip) { m_states.push_back(_clip); };

		void Init(int _startIdx);
		void Transition(int _newIdx, float _duration = 0.5f);
		void Update(float _dt, const Skeleton& _skeleton, LocalPoseTRS& _outPose);
		bool IsValid() const { return m_states.size() > 0 && m_bIsInitialized; }

		void SetInputAxis(const Vector2& _axis) { m_InputAxis = _axis; }
		std::shared_ptr<BlendClip> GetClip(int _idx) { return m_states[_idx]; }

	private:
		bool m_bIsInitialized{ false };
		int m_curStateIdx{ -1 };
		int m_prevIdx{ -1 }; // 트랜지션일 때 페이드 인 아웃용 직전 스테이트
		Vector2 m_InputAxis;

		std::vector<std::shared_ptr<BlendClip>> m_states;

		float m_fadeElapsed{ 0.0f };
		float m_fadeDuration{ 0.0f };	// 0 = 페이드 없음
		bool IsFading() const { return m_fadeDuration > 0.0f; }

		LocalPoseTRS m_posePrev, m_poseNext; // 트랜지션 간 각각 사용
	};
}