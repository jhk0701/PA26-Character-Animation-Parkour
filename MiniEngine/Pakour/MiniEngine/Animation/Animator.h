#pragma once
#include "Asset/AnimClip.h"
#include "Asset/RootMotion.h"

namespace MiniEngine 
{
	class IAnimatorClip;
	class ActionClip;
	class SkeletalMeshComponent;

	// 다수의 클립들을 연결 및 제어하는 FSM
	// 클립 간 트랜지션 필요
	class AnimStateMachine
	{
	public:
		AnimStateMachine() {};
		~AnimStateMachine() { m_states.clear(); }

		void Reserve(uint8_t _cnt) { m_states.reserve(_cnt); }
		void AddState(std::shared_ptr<IAnimatorClip> _clip) { m_states.push_back(_clip); };
		void Transition(int _newIdx, float _duration = 0.5f);
		void Update(float _dt, const Skeleton& _skeleton);
		
		const LocalPoseTRS& GetPoseTRS() const { return m_poseTarget; }

	private:
		int m_curStateIdx{ -1 };
		int m_prevIdx{ -1 }; // 트랜지션일 때 페이드 인 아웃용 직전 스테이트

		std::vector<std::shared_ptr<IAnimatorClip>> m_states;

		// TODO : Transition 정보 및 조건
		float m_fadeElapsed{ 0.0f };
		float m_fadeDuration{ 0.0f };	// 0 = 페이드 없음

		LocalPoseTRS m_poseTarget; // 최종 타겟
		LocalPoseTRS m_posePrev; // 트랜지션 중인 경우, 지나오는 애니메이션 스테이트에 사용

		bool IsFading() const { return m_fadeDuration > 0.0f; }
	};

	// Animator : 애니메이션 총괄 관리
	class Animator
	{
		struct Layer 
		{
			bool m_bIsPlaying{ false };
			LocalPoseTRS m_layerPose;				// 재생한 클립의 좌표가 이 값에 저장될 것

			std::shared_ptr<IAnimatorClip> m_pClip; // 재생할 클립 타입 // 여러 개 사용될 것 - 테스트용 1개
		};

	public:
		Animator();
		Animator(std::shared_ptr<SkeletalMeshComponent> _meshComp);
		~Animator() {};

		void Init();
		void Update(float _dt);

		void SampleBaseLayer(float _dt);
		void SampleOverrideLayer(float _dt);
		void FinalizePose();

		void AddLocomotion(std::shared_ptr<IAnimatorClip>& _loco) { m_baseLayer.m_pClip = _loco; }
		void PlayActionClip(std::shared_ptr<ActionClip>& _action, float _fadeDuration = 0.5f);

		void SetEnableRootMotion(bool _bEnable) { m_bEnableRootMotion = _bEnable; }
		void SetRootBoneIdx(int _idx) { m_rootBoneIdx = _idx; }
		void SetRootMotionConfig(RootMotionConfig& _config) { m_rootMotionCfg = _config; }
		
		const RootMotionDelta& GetRootMotionDelta() const { return m_rootMotionDt; }
		const RootMotionConfig& GetRootMotionConfig() const { return m_rootMotionCfg; }
		
		RootMotionDelta ConsumeRootMotionDelta(); // 델타를 읽고 0 으로 비움
		bool IsActionClipPlaying() const { return m_overrideLayer.m_bIsPlaying; }

	private:
		std::weak_ptr<SkeletalMeshComponent> m_meshComp; // 메시 컴포넌트 약참조
		const Skeleton& GetSkeleton() const;

		// 레이어간 페이드 인아웃
		float m_fadeElapsed		= 0.0f;
		float m_fadeDuration	= 0.0f;	// 0 = 페이드 없음
		float m_actionDuration	= 0.0f;
		float m_actionElapsed	= 0.0f;
		float m_actionEndTime	= 0.0f; // 종료 페이드가 시작되는 시점

		LocalPoseTRS m_poseTarget;		// 블렌드, 트랜지션 등 연산이 반영되는 본 위계구조
		std::vector<Matrix> m_localPose;	// 합성 로컬 행렬 스크래치
		std::vector<Matrix>* m_pBoneMatrices;	// 스키닝 최종 행렬

		bool m_bEnableRootMotion{ false };
		int m_rootBoneIdx{ 0 };
		RootMotionDelta m_rootMotionDt;
		RootMotionConfig m_rootMotionCfg;

		Layer m_baseLayer;		// 레거시
		AnimStateMachine m_baseTrack; // 로코모션 루프용

		Layer m_overrideLayer;	// 단발 액션 오버라이드용
	};
}