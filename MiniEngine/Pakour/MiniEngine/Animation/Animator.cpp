#include "pch.h"
#include "Animation/Animator.h"
#include "Core/Log.h"
#include "Animation/IAnimatorClip.h"
#include "Animation/ActionClip.h"
#include "Scene/Actor.h"
#include "Scene/SkeletalMeshComponent.h"

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
			m_states[m_curStateIdx]->Sample(_dt, _skeleton, _outPose); // 포즈 타겟에 해당 index 바로 샘플링
			return;
		}

		m_fadeElapsed += _dt;
		
		m_states[m_curStateIdx]->Sample(_dt, _skeleton, m_poseNext); // 메인 포즈
		m_states[m_prevIdx]->Sample(_dt, _skeleton, m_posePrev); // 이전 포즈

		const float w = m_fadeElapsed / m_fadeDuration;
		BlendPose(m_posePrev, m_poseNext, w, _outPose);
	}

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
		const Skeleton& skeleton = GetSkeleton();

		if (m_baseTrack.IsValid() == false)
			return;

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

		if (m_bEnableRootMotion)
		{
			m_rootMotionDt.Reset();
			ExtractClipRootMotion(*m_overrideTrack.m_pClip->GetClip(), skeleton, t0, t1, m_rootMotionDt, m_rootBoneIdx);
		}

		// 액션 클립 재생
		m_overrideTrack.m_actionElapsed = t1;
		m_overrideTrack.m_fadeElapsed += m_overrideTrack.IsEndArea() ? -_dt : _dt;
		m_overrideTrack.m_pClip->Sample(_dt, skeleton, m_overrideTrack.m_layerPose);	// m_poseTarget

		float w = m_overrideTrack.GetProgress();
		w = std::clamp(w, 0.0f, 1.0f);

		BlendPose(m_poseTarget, m_overrideTrack.m_layerPose, w, m_poseTarget);

		if (m_overrideTrack.IsEnd())
		{
			m_overrideTrack.m_bIsPlaying = false;
			m_overrideTrack.m_pClip = nullptr;
		}
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

	void Animator::PlayActionClip(std::shared_ptr<ActionClip>& _action, float _fadeDuration)
	{
		if (m_overrideTrack.m_bIsPlaying)
			return;

		m_overrideTrack.m_bIsPlaying = true;
		m_overrideTrack.m_pClip = _action;
		m_overrideTrack.m_fadeDuration = _fadeDuration;
		m_overrideTrack.m_fadeElapsed = 0.0f;
		m_overrideTrack.m_actionElapsed = 0.0f;
		m_overrideTrack.m_actionDuration = _action->GetDuration();

		m_overrideTrack.m_actionEndTime = m_overrideTrack.m_actionDuration - _fadeDuration * (1.0f / _action->GetTickPerSec());
		if (m_overrideTrack.m_actionEndTime < 0.0f)
			m_overrideTrack.m_actionEndTime = m_overrideTrack.m_actionDuration - 0.01f;

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

	void Animator::SetBaseTrackInputAxis(const Vector2& _axis)
	{
		m_baseTrack.SetInputAxis(_axis);
	}

	const Skeleton& Animator::GetSkeleton() const
	{
		return m_meshComp.lock()->GetMesh().lock()->GetSkeleton();
	}
}
