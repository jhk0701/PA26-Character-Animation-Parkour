#include "pch.h"
#include "Animation/Animator.h"
#include "Animation/IAnimatorClip.h"
#include "Animation/ActionClip.h"
#include "Scene/Actor.h"
#include "Scene/SkeletalMeshComponent.h"

namespace MiniEngine 
{
	void AnimStateMachine::Transition(int _newIdx, float _duration)
	{
		m_fadeDuration = _duration;
		m_prevIdx = m_curStateIdx;
		m_curStateIdx = _newIdx;
	}
	void AnimStateMachine::Update(float _dt, const Skeleton& _skeleton)
	{
		if (m_curStateIdx < 0 || m_curStateIdx >= m_states.size())
			return;

		if (IsFading())
		{


		}
		else
			m_states[m_curStateIdx]->Sample(_dt, _skeleton, m_poseTarget); // 포즈 타겟에 해당 index 바로 샘플링
	}

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

		SampleBaseLayer(_dt);

		SampleOverrideLayer(_dt);
		
		FinalizePose();
	}

	void Animator::SampleBaseLayer(float _dt)
	{
		const Skeleton& skeleton = GetSkeleton();

		// 로코모션 관리 고도화
		if (m_baseLayer.m_bIsPlaying == false ||
			m_baseLayer.m_pClip == nullptr)
			return;

		m_baseLayer.m_pClip->Sample(_dt, skeleton, m_baseLayer.m_layerPose);	// m_poseTarget
	}

	void Animator::SampleOverrideLayer(float _dt)
	{
		if (m_overrideLayer.m_bIsPlaying == false || 
			m_overrideLayer.m_pClip == nullptr)
		{
			m_poseTarget = m_baseLayer.m_layerPose;
			return;
		}

		const Skeleton& skeleton = GetSkeleton();

		// root motion 추출
		const float t0 = m_actionElapsed;
		float t1 = m_actionElapsed + _dt;

		if (m_bEnableRootMotion)
		{
			m_rootMotionDt.Reset();
			ExtractClipRootMotion(*m_overrideLayer.m_pClip->GetClip(), skeleton, t0, t1, m_rootMotionDt, m_rootBoneIdx);
		}

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

	const Skeleton& Animator::GetSkeleton() const
	{
		return m_meshComp.lock()->GetMesh().lock()->GetSkeleton();
	}
}
