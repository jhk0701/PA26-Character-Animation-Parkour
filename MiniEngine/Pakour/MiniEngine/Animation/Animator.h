#pragma once
#include "Animation/AnimStateMachine.h"
#include "Asset/AnimClip.h"
#include "Asset/RootMotion.h"
#include <functional>

namespace MiniEngine 
{
	class BlendClip;
	class ActionClip;
	class SkeletalMeshComponent;

	struct AnimLayer
	{
		bool m_bIsPlaying{ false };
		uint8_t m_curPriority = 0; // 높을수록 바로 재생

		// 레이어간 페이드 인아웃
		float m_fadeElapsed = 0.0f;
		float m_fadeDuration = 0.0f;	// 0 = 페이드 없음
		float m_actionDuration = 0.0f;
		float m_actionElapsed = 0.0f;
		float m_actionEndTime = 0.0f; // 종료 페이드가 시작되는 시점
	
		std::shared_ptr<ActionClip> m_pClip; // 재생할 클립 타입 // 여러 개 사용될 것 - 테스트용 1개
		LocalPoseTRS m_layerPose;				// 재생한 클립의 좌표가 이 값에 저장될 것

		std::function<void()> m_onClipStarted;
		std::function<void()> m_onClipEnded;

		bool IsEnd() const { return m_actionElapsed >= m_actionDuration; }
		bool IsEndArea() const { return m_actionElapsed >= m_actionEndTime; }
		float GetProgress() const { return m_fadeElapsed / m_fadeDuration; }
	};

	// Animator : 애니메이션 총괄 관리
	class Animator
	{
	public:
		Animator(std::shared_ptr<SkeletalMeshComponent> _meshComp);
		virtual ~Animator() {};

		void Init(int _baseTrackStart);
		void Update(float _dt);

		void SampleBaseTrack(float _dt);
		void SampleOverrideTrack(float _dt);
		void FinalizePose();

		

		// 루트모션 api
		void SetEnableRootMotion(bool _bEnable) { m_bEnableRootMotion = _bEnable; }
		void SetRootBoneIdx(int _idx) { m_rootBoneIdx = _idx; }
		void SetRootMotionConfig(RootMotionConfig& _config) { m_rootMotionCfg = _config; }
		const bool GetIsEnableRootMotion() const { return m_bEnableRootMotion; }
		const RootMotionDelta& GetRootMotionDelta() const { return m_rootMotionDt; }
		const RootMotionConfig& GetRootMotionConfig() const { return m_rootMotionCfg; }
		RootMotionDelta ConsumeRootMotionDelta(); // 델타를 읽고 0 으로 

		// base track api
		void SetBaseTrackInputAxis(const Vector2& _axis);
		void TranstionBaseTrack(int _nextIdx, float _duration = 0.5f);
		void ReserveBaseLocomotion(uint8_t _cnt) { m_baseTrack.Reserve(_cnt); }
		void AddBaseLocomotion(std::shared_ptr<BlendClip>& _loco) { m_baseTrack.AddState(_loco); }

		// override track api
		void PlayActionClip(std::shared_ptr<ActionClip>& _action, float _fadeDuration = 0.5f, uint8_t _priority = 0);
		void StopActionClip();

		// 단순 재생 여부 확인
		bool IsActionClipPlaying() const { return m_overrideTrack.m_bIsPlaying; }
		// 특정 우선순위 이상이 재생중인지 확인
		bool IsActionClipPlaying(uint8_t _priority) const 
		{ 
			return m_overrideTrack.m_bIsPlaying && 
				m_overrideTrack.m_curPriority >= _priority;
		}

		void SetOverrideTrackStartEvent(std::function<void()>&& _event) { m_overrideTrack.m_onClipStarted = _event; }
		void SetOverrideTrackEndEvent(std::function<void()>&& _event) { m_overrideTrack.m_onClipEnded = _event; }

	private:
		bool m_bIsInitialized{ false };

		std::weak_ptr<SkeletalMeshComponent> m_meshComp; // 메시 컴포넌트 약참조
		const Skeleton& GetSkeleton() const;

		LocalPoseTRS m_poseTarget;		// 블렌드, 트랜지션 등 연산이 반영되는 본 위계구조
		std::vector<Matrix> m_localPose;	// 합성 로컬 행렬 스크래치
		std::vector<Matrix>* m_pBoneMatrices;	// 스키닝 최종 행렬

		bool m_bEnableRootMotion{ false };
		int m_rootBoneIdx{ 0 };
		RootMotionDelta m_rootMotionDt;
		RootMotionConfig m_rootMotionCfg;

		AnimStateMachine m_baseTrack; // 로코모션 루프용
		AnimLayer m_overrideTrack;	// 단발 액션 오버라이드용
	};
}