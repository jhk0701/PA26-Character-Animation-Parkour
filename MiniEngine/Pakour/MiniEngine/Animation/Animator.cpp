#include "pch.h"
#include "Animation/Animator.h"
#include "Animation/IAnimatorClip.h"
#include "Animation/ActionClip.h"
#include "Scene/Actor.h"
#include "Scene/SkeletalMeshComponent.h"

namespace MiniEngine 
{
	float ClipDurationSec(const AnimClip* _clip) 
	{
		const float tps = (_clip->ticksPerSecond > 0.0f) ? _clip->ticksPerSecond : 1.0f;
		return _clip->duration / tps;
	};

	Animator::Animator() { Init(); }
	Animator::Animator(std::shared_ptr<SkeletalMeshComponent> _meshComp) : m_meshComp(_meshComp)
	{
		Init();
		m_pBoneMatrices = m_meshComp.lock()->GetBoneMatricesPtr();
	}

	void Animator::Init()
	{
		m_fadeDuration = 0.0f;

		m_baseLayer.m_bIsPlaying = true;
		m_overrideLayer.m_bIsPlaying = false;
	}

	void Animator::Update(float _dt)
	{
		if (m_meshComp.expired())
		{
			m_pBoneMatrices->clear();
			return;
		}

		std::shared_ptr<SkinnedMesh> pSkin = m_meshComp.lock()->GetMesh().lock();
		const Skeleton& skeleton = pSkin->GetSkeleton();

		if (m_bEnableRootMotion)
			m_rootMotionDt.Reset();

		// TODO : 로코모션 관리 고도화
		if (m_baseLayer.m_pClip)
			m_baseLayer.m_pClip->Sample(_dt, skeleton, m_baseLayer.m_layerPose);	// m_poseTarget

		if (m_overrideLayer.m_bIsPlaying &&
			m_overrideLayer.m_pClip)
		{
			// root motion 추출
			const float t0 = m_actionElapsed;
			float t1 = m_actionElapsed + _dt;

			if (m_bEnableRootMotion)
				ExtractClipRootMotion(*m_overrideLayer.m_pClip->GetClip(), skeleton, t0, t1, m_rootMotionDt, m_rootBoneIdx);

			// 액션 클립 재생
			m_actionElapsed = t1;
			m_fadeElapsed += m_actionElapsed < m_actionEndTime ? _dt : -_dt;
			m_overrideLayer.m_pClip->Sample(_dt, skeleton, m_overrideLayer.m_layerPose);	// m_poseTarget

			float w = 0.0f;
			w = m_fadeElapsed / m_fadeDuration;
			w = std::clamp(w, 0.0f, 1.0f);

			BlendPose(m_baseLayer.m_layerPose, m_overrideLayer.m_layerPose, w, m_poseTarget);

			if (m_actionDuration < m_actionElapsed)
			{
				m_overrideLayer.m_bIsPlaying = false;
				m_overrideLayer.m_pClip = nullptr;
			}
		}
		else 
			m_poseTarget = m_baseLayer.m_layerPose;
		
		// 포즈 적용 절차
		if (m_bEnableRootMotion) 
		{
			// 루트 본으로부터 바인드된 Transform 제거
			// 추출한 루트 모션 델타값은 외부에서 소비할 것
			StripRootMotionFromPose(skeleton, m_rootMotionCfg, m_poseTarget, m_rootBoneIdx);
		}

		ComposePose(m_poseTarget, m_localPose);
		skeleton.ComputeBoneMatrices(m_localPose, *m_pBoneMatrices);
	}

	void Animator::PlayActionClip(std::shared_ptr<ActionClip>& _action, float _fadeDuration)
	{
		if (m_overrideLayer.m_bIsPlaying)
			return;

		m_overrideLayer.m_bIsPlaying = true;
		m_overrideLayer.m_pClip = _action;

		m_fadeDuration = _fadeDuration;
		m_fadeElapsed = 0.0f;
		
		m_actionElapsed = 0.0f;
		m_actionDuration = _action->GetDuration();
		m_actionEndTime = m_actionDuration - _fadeDuration * (1.0f / _action->GetTickPerSec());
		if (m_actionEndTime < 0.0f)
			m_actionEndTime = m_actionDuration - 0.01f;

		_action->Play();
	}

	RootMotionDelta Animator::ConsumeRootMotionDelta()
	{
		if (m_rootMotionCfg.applyY == false) 
			m_rootMotionDt.translation = Vector3(0.0f);

		if (m_rootMotionCfg.applyYaw == false)
			m_rootMotionDt.rotation = Quaternion(0.0f, 0.0f, 0.0f, 1.0f);
		
		const RootMotionDelta delta = m_rootMotionDt;
		m_rootMotionDt.Reset();
		return delta;
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
