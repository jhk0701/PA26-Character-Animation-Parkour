#pragma once
#include "Asset/AnimClip.h"
#include "Asset/RootMotion.h"

namespace MiniEngine 
{
	class IAnimatorClip;
	class ActionClip;
	class SkeletalMeshComponent;

	class Animator
	{
		struct Layer 
		{
			bool m_bIsPlaying{ false };

			std::shared_ptr<IAnimatorClip> m_pClip; // 재생할 클립 타입 // 여러 개 사용될 것 - 테스트용 1개
			LocalPoseTRS m_layerPose; // 재생한 클립의 좌표가 이 값에 저장될 것
		};

	public:
		Animator();
		Animator(std::shared_ptr<SkeletalMeshComponent> _meshComp);
		~Animator() {};

		void Init();
		void Update(float _dt);

		// 클립 재생/전환. _clipIndex 범위 밖(-1 포함) = 정지(바인드 포즈).
		// _fadeSec>0 이고 현재 재생 중이면 크로스페이드, 아니면 즉시 전환.
		// 정지 대상(-1)으로도 페이드 가능(바인드 포즈로 블렌드).
		// void PlayClip(int _clipIndex, float _fadeSec = 0.0f);
		// void SetActiveClip(int _clipIndex) { PlayClip(_clipIndex, 0.0f); }
		// bool IsFading() const { return m_fadeDuration > 0.0f; }

	private:
		std::weak_ptr<SkeletalMeshComponent> m_meshComp; // 메시 컴포넌트 약참조

		float m_fadeElapsed		= 0.0f;
		float m_fadeDuration	= 0.0f;		// 0 = 페이드 없음
		float m_actionDuration = 0.0f;
		float m_actionElapsed	= 0.0f;
		float m_actionEndTime	= 0.0f; // 종료 페이드가 시작되는 시점

		LocalPoseTRS m_poseTarget;				// 블렌드, 트랜지션 등 연산이 반영되는 본 위계구조
		std::vector<Matrix> m_localPose;		// 합성 로컬 행렬 스크래치
		std::vector<Matrix>* m_pBoneMatrices;	// 스키닝 최종 행렬
		
		Layer m_baseLayer;		// 로코모션 루프용
		Layer m_overrideLayer;	// 단발 액션 오버라이드용

		// tag - action 맵
		std::unordered_map<uint8_t, std::shared_ptr<ActionClip>> m_mapActions;

		bool m_bEnableRootMotion{ false };
		
		int m_rootBoneIdx{ 0 };
		RootMotionDelta m_rootMotionDt;
		RootMotionConfig m_rootMotionCfg;

	public:
		void AddLocomotion(std::shared_ptr<IAnimatorClip>& _loco) { m_baseLayer.m_pClip = _loco; }
		void PlayActionClip(std::shared_ptr<ActionClip>& _action, float _fadeDuration = 0.5f);

		void SetEnableRootMotion(bool _bEnable) { m_bEnableRootMotion = _bEnable; }
		void SetRootBoneIdx(int _idx) { m_rootBoneIdx = _idx; }
		void SetRootMotionConfig(bool _bExtractY, bool _bExtractYaw) 
		{
			m_rootMotionCfg.extractY = _bExtractY;
			m_rootMotionCfg.extractYaw = _bExtractYaw;
		}

		const RootMotionDelta& GetRootMotionDelta() const { return m_rootMotionDt; }
		RootMotionDelta ConsumeRootMotionDelta(); // 델타를 읽고 0 으로 비움
	};
}