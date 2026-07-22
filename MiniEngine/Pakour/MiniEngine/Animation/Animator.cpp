#include "pch.h"
#include "Animation/Animator.h"
#include "Core/Log.h"
#include "Animation/BlendClip.h"
#include "Animation/ActionClip.h"
#include "Scene/Actor.h"
#include "Scene/SkeletalMeshComponent.h"

namespace MiniEngine 
{
	Animator::Animator(std::shared_ptr<SkeletalMeshComponent> _meshComp) : m_meshComp(_meshComp)
	{
		m_pBoneMatrices = m_meshComp.lock()->GetBoneMatricesPtr();
	}

	void Animator::Init(int _baseTrackStart)
	{
		m_bIsInitialized = true;

		m_baseTrack.Init(_baseTrackStart);
		m_overrideTrack.m_bIsPlaying = false;
	}

	void Animator::Update(float _dt)
	{
		if (m_bIsInitialized == false)
		{
			MG_LOG_INFO("[Animator Error] Animator is not initialized.");
			return;
		}

		if (m_meshComp.expired())
		{
			m_pBoneMatrices->clear();
			return;
		}

		SampleBaseTrack(_dt);
		SampleOverrideTrack(_dt);
		FinalizePose();
	}

	void Animator::SampleBaseTrack(float _dt)
	{
		if (m_baseTrack.IsValid() == false)
			return;

		const Skeleton& skeleton = GetSkeleton();

		m_baseTrack.Update(_dt, skeleton, m_poseTarget);
	}

	void Animator::SampleOverrideTrack(float _dt)
	{
		if (m_overrideTrack.m_bIsPlaying == false ||
			m_overrideTrack.m_pClip == nullptr)
			return;

		const Skeleton& skeleton = GetSkeleton();

		// root motion 추출
		const float t0 = m_overrideTrack.m_actionElapsed;
		float t1 = m_overrideTrack.m_actionElapsed + _dt;

		if (m_bEnableRootMotion && 
			m_overrideTrack.m_pClip->IsApplyingRootBone())
		{
			m_rootMotionDt.Reset();
			ExtractClipRootMotion(*m_overrideTrack.m_pClip->GetClip(), skeleton, t0, t1, m_rootMotionDt, m_rootBoneIdx);
		}

		m_overrideTrack.m_actionElapsed = t1;
		
		// 페이드 인아웃 처리
		if (m_overrideTrack.IsEndArea())
			m_overrideTrack.m_fadeElapsed -= _dt; // 페이드 아웃
		else if (m_overrideTrack.m_fadeElapsed < m_overrideTrack.m_fadeDuration)
			m_overrideTrack.m_fadeElapsed += _dt; // 페이드 인
		
		// 액션 클립 재생
		AnimNotifyParam notifyParam;
		notifyParam.m_pActor = m_meshComp.lock()->owner.lock().get();
		m_overrideTrack.m_pClip->Sample(_dt, skeleton, m_overrideTrack.m_layerPose, notifyParam);	// m_poseTarget

		float w = m_overrideTrack.GetProgress();
		w = std::clamp(w, 0.0f, 1.0f);

		BlendPose(m_poseTarget, m_overrideTrack.m_layerPose, w, m_poseTarget);

		if (m_overrideTrack.IsEnd())
			StopActionClip();
	}

	void Animator::FinalizePose()
	{
		// 애니메이션 샘플링이 끝난 포즈 적용 절차
		const Skeleton& skeleton = GetSkeleton();
		
		if (m_bEnableRootMotion)
		{
			// 루트 본으로부터 바인드된 Transform 제거
			// 추출한 루트 모션 델타값은 외부에서 소비할 것
			StripRootMotionFromPose(skeleton, m_rootMotionCfg, m_poseTarget, m_rootBoneIdx);
		}

		ComposePose(m_poseTarget, m_localPose);
		skeleton.ComputeBoneMatrices(m_localPose, *m_pBoneMatrices);
	}

	void Animator::PlayActionClip(std::shared_ptr<ActionClip>& _action, float _fadeDuration, uint8_t _priority)
	{
		if (m_overrideTrack.m_bIsPlaying && m_overrideTrack.m_curPriority >= _priority)
			return;

		m_overrideTrack.m_bIsPlaying = true;
		m_overrideTrack.m_curPriority = _priority;
		m_overrideTrack.m_pClip = _action;
		m_overrideTrack.m_fadeDuration = _fadeDuration;
		m_overrideTrack.m_fadeElapsed = 0.0f;
		m_overrideTrack.m_actionElapsed = 0.0f;
		m_overrideTrack.m_actionDuration = _action->GetDuration();

		m_overrideTrack.m_actionEndTime = m_overrideTrack.m_actionDuration - _fadeDuration;
		if (m_overrideTrack.m_actionEndTime < 0.0f)
			m_overrideTrack.m_actionEndTime = m_overrideTrack.m_actionDuration - 0.01f;

		_action->Play();

		if (m_overrideTrack.m_onClipStarted)
			m_overrideTrack.m_onClipStarted();
	}

	void Animator::StopActionClip()
	{
		m_overrideTrack.m_bIsPlaying = false;
		m_overrideTrack.m_pClip = nullptr;
		m_overrideTrack.m_curPriority = 0;

		if (m_overrideTrack.m_onClipEnded)
			m_overrideTrack.m_onClipEnded();
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

	void Animator::SetBaseTrackInputAxis(const Vector2& _axis)
	{
		m_baseTrack.SetInputAxis(_axis);
	}

	void Animator::TranstionBaseTrack(int _nextIdx, float _duration)
	{
		m_baseTrack.Transition(_nextIdx, _duration);
	}

	const Skeleton& Animator::GetSkeleton() const
	{
		return m_meshComp.lock()->GetMesh().lock()->GetSkeleton();
	}
}
