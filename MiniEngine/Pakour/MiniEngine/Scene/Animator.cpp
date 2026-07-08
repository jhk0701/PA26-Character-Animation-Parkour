#include "pch.h"
#include "Scene/Animator.h"
#include "Scene/SkeletalMeshComponent.h"
#include "Asset/BlendClip.h"

namespace MiniEngine 
{
	Animator::Animator() { Init(); }
	Animator::Animator(std::shared_ptr<SkeletalMeshComponent> _meshComp) : m_meshComp(_meshComp)
	{
		Init();
		m_pBoneMatrices = m_meshComp.lock()->GetBoneMatricesPtr();
	}

	void Animator::Init()
	{
		m_playTime = 0.0f;
		m_targetTime = 0.0f;
		m_fadeDuration = 0.0f;
		m_fadeElapsed = 0.0f;
	}

	void Animator::Update(float _dt)
	{
		if (m_meshComp.expired())
		{
			m_pBoneMatrices->clear();
			return;
		}

		m_playTime += _dt;

		std::shared_ptr<SkinnedMesh> pSkin = m_meshComp.lock()->GetMesh().lock();
		const Skeleton& skeleton = pSkin->GetSkeleton();

		// 각 클립들에 대해 SamplePose
		if (m_loco)
			m_loco->Sample(_dt, skeleton, m_poseTarget);
		else
		{
			// 테스트
			pSkin->GetClips()[2].SampleTRS(m_playTime, skeleton, m_poseTarget);
			// SampleBindPoseTRS(m_mesh->GetSkeleton(), _outPose); // 클립 없음 — 바인드 포즈
		}

		ComposePose(m_poseTarget, m_localPose);

		skeleton.ComputeBoneMatrices(m_localPose, *m_pBoneMatrices);
	}
}

// _clipIndex 가 유효 클립이면 SampleTRS, 아니면 바인드 포즈 TRS.
//void SkeletalMeshComponent::SamplePose(int _clipIndex, float _timeSec, LocalPoseTRS& _outPose) const
//{
//	if (_clipIndex >= 0 && _clipIndex < static_cast<int>(m_mesh->GetClips().size()))
//		m_mesh->GetClips()[_clipIndex].SampleTRS(_timeSec, m_mesh->GetSkeleton(), _outPose);
//	else
//		SampleBindPoseTRS(m_mesh->GetSkeleton(), _outPose); // 클립 없음 — 바인드 포즈
//}

//// 본 행렬을 현재 재생 시간(+페이드 가중치) 기준으로 다시 계산.
//void SkeletalMeshComponent::RefreshBoneMatrices()
//{
//    if (!m_mesh)
//    {
//        m_boneMatrices.clear();
//        return;
//    }

//    const Skeleton& skeleton = m_mesh->GetSkeleton();

//    SamplePose(m_activeClip, m_playTime, m_poseA);

//    if (IsFading())
//    {
//        // 현재 → 대상 크로스페이드: TRS 가중 보간(pos/scale Lerp, rot Slerp).
//        const float weight = m_fadeElapsed / m_fadeDuration;

//        SamplePose(m_targetClip, m_targetTime, m_poseB);

//        BlendPose(m_poseA, m_poseB, weight, m_poseA);
//    }

//    ComposePose(m_poseA, m_localPose);
//    
//    skeleton.ComputeBoneMatrices(m_localPose, m_boneMatrices);
//}

//void SkeletalMeshComponent::PlayClip(int _clipIndex, float _fadeSec)
//{
//	// 현재가 정지(-1)이거나 페이드 시간이 없으면 즉시 전환(기존 SetActiveClip 동작).
//	if (_fadeSec <= 0.0f || m_activeClip < 0)
//	{
//		m_activeClip = _clipIndex;
//		m_playTime = 0.0f;
//		m_targetClip = -1;
//		m_targetTime = 0.0f;
//		m_fadeDuration = 0.0f;
//		m_fadeElapsed = 0.0f;
//		return;
//	}
//
//	// 크로스페이드 시작(-1 대상이면 바인드 포즈로 페이드).
//	// 페이드 중 재호출이면 현재 진행 포즈 기준으로 새 페이드를 시작하는 대신
//	// 대상만 교체(단순화 — 시각적으로 충분히 부드러움).
//	m_targetClip = _clipIndex;
//	m_targetTime = 0.0f;
//	m_fadeDuration = _fadeSec;
//	m_fadeElapsed = 0.0f;
//}