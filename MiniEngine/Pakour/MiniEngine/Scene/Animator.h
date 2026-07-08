#pragma once
#include "Asset/AnimClip.h"

namespace MiniEngine 
{
	class SkeletalMeshComponent;
	class BlendClip;

	class Animator
	{
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

		// 재생할 클립 인덱스(-1 = 정지, 바인드 포즈 유지). 즉시 전환 = PlayClip(i, 0).
		// void SetActiveClip(int _clipIndex) { PlayClip(_clipIndex, 0.0f); }
		// int  GetActiveClip() const { return m_activeClip; }
		// const int GetClipCount() const { return m_mesh->GetClips().size(); }

		// 크로스페이드 진행 중인지(에디터 UI 표시용).
		// bool IsFading() const { return m_fadeDuration > 0.0f; }

	private:
		std::weak_ptr<SkeletalMeshComponent> m_meshComp; // 메시 컴포넌트 약참조

		float m_playTime = 0.0f;
		float m_targetTime = 0.0f;      // 대상 클립 재생 시간(초)
		float m_fadeDuration = 0.0f;    // 0 = 페이드 없음
		float m_fadeElapsed = 0.0f;

		LocalPoseTRS m_poseTarget; // 블렌드, 트랜지션 등 연산이 반영되는 본 위계구조
		std::vector<Matrix> m_localPose;    // 합성 로컬 행렬 스크래치
		std::vector<Matrix>* m_pBoneMatrices; // 스키닝 최종 행렬
		
		// Base Layer : 로코모션
		std::shared_ptr<BlendClip> m_loco;
		// std::vector<std::shared_ptr<BlendClip>> m_locomotions;

	public:
		void SetLocomotion(std::shared_ptr<BlendClip>& _loco) { m_loco = _loco; }
	};
}