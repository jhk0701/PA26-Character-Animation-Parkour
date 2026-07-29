#include "pch.h"
#include "Animation/Animator.h"
#include "Animation/BlendClip.h"
#include "Animation/ActionClip.h"
#include "Scene/Actor.h"
#include "Scene/SkeletalMeshComponent.h"

#include "Asset/IK.h"
#include "Core/Log.h"

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
			m_globalPose.clear();
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

		m_baseTrack.Update(_dt, GetSkeleton(), m_poseTarget);
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
		skeleton.ComputeGlobalPose(m_localPose, m_globalPose);
		skeleton.ApplyInverseBind(m_globalPose, *m_pBoneMatrices); // 저장된 값을 스키닝 행렬에 반영
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
	
	// IK 관련
	void Animator::SetIKGoal(const TwoBoneIKBinding& _binding, const IKGoal& _goal)
	{
		const HumanoidBoneMap& BONE_MAP = m_meshComp.lock()->GetMesh().lock()->GetHumanoidBones();
		if (!BONE_MAP.Has(_binding.upper) || !BONE_MAP.Has(_binding.lower) || !BONE_MAP.Has(_binding.end))
		{
			MG_LOG_WARN("[Animator]::SetIKGoal Warning:: _binding bone is not included");
			return;
		}
		m_mapIKBinding[_binding.end] = { _binding, _goal };

		RefreshIKAny();
	}

	void Animator::ClearIKGoal(HumanoidBone _end)
	{
		if (_end == HumanoidBone::None || _end >= HumanoidBone::Count) 
			return;

		m_mapIKBinding[_end].Reset();
		RefreshIKAny();
	}

	void Animator::ClearAllIKGoals()
	{
		for (auto it = m_mapIKBinding.begin(); it != m_mapIKBinding.end(); ++it)
			it->second.Reset();

		m_ikPelvisOffset = Vector3(0.0f);
		m_bUseIK = false;
	}

	const IKGoal* Animator::GetIKGoal(HumanoidBone _end) const
	{
		if (_end == HumanoidBone::None || _end >= HumanoidBone::Count)
			return nullptr;

		const auto IT = m_mapIKBinding.find(_end);
		if (IT == m_mapIKBinding.end())
			return nullptr;

		const IKGoal& g =  IT->second.goal;
		return (g.positionAlpha > 0.0f || g.rotationAlpha > 0.0f) ? &g : nullptr;
	}

	void Animator::SetIKPelvisOffset(const Vector3& _offsetMeshLocal)
	{
		m_ikPelvisOffset = _offsetMeshLocal;
		RefreshIKAny();
	}

	void Animator::SolveIKAndRefresh(const SkinnedMesh* _mesh)
	{
		if (!m_bUseIK || !_mesh)
			return;

		if (m_localPose.empty() || m_localPose.size() != _mesh->GetSkeleton().bones.size())
			return;

		// 애니메이션 적용 후 포즈 불러오기
		// 재연산하지 않고 보존된 결과값만 가져옴
		const Skeleton& skeleton = _mesh->GetSkeleton();
		skeleton.ComputeGlobalPose(m_localPose, m_globalPose);
		
		// two bone ik만 있으므로 바로 적용
		// 다른 종류의 ik가 생기면 추후 이 부분을 수정
		ApplyIKGoals(skeleton, _mesh->GetHumanoidBones());
		
		// 스켈레톤에 최종 바인드
		skeleton.ApplyInverseBind(m_globalPose, *m_pBoneMatrices);
	}

	void Animator::ResetForMesh()
	{
		ClearAllIKGoals();
	}

	void Animator::BindGlobal(int _boneIdx, const Skeleton& _inSkeleton, Matrix& _out) const
	{
		_out = _inSkeleton.bones[_boneIdx].localBindPose;

		for (int p = _inSkeleton.bones[_boneIdx].parentIndex; p >= 0; p = _inSkeleton.bones[p].parentIndex)
			_out = _out * _inSkeleton.bones[p].localBindPose;
	}

	bool Animator::BindPole(const Skeleton& _skeleton, int _upperBone, int _lowerBone, int _endBone, Vector3& _outPole) const
	{
		const int count = static_cast<int>(_skeleton.bones.size());
		if (_upperBone < 0 || _lowerBone < 0 || _endBone < 0 ||
			_upperBone >= count || _lowerBone >= count || _endBone >= count) 
			return false;

		Matrix bindU;
		Matrix bindL;
		Matrix bindE;
		
		BindGlobal(_upperBone, _skeleton, bindU);
		BindGlobal(_lowerBone, _skeleton, bindL);
		BindGlobal(_endBone, _skeleton, bindE);

		Vector3 bindPole;
		TwoBoneIKBone bone;
		bone.upperPos = bindU.Translation();
		bone.lowerPos = bindL.Translation();
		bone.endPos = bindE.Translation();

		if (!FallbackPole(bone, bindPole))
			return false; // 바인드조차 완전히 편 경우 -> false

		// 굽힌 방향을 upper 바인드 로컬로
		// 현재 upper 글로벌로 재적용
		const Vector3 DIR_BIND_LOCAL = Vector3::Transform(bindPole - bindU.Translation(), bindU.Invert());
		const Matrix& CUR_U = m_globalPose[static_cast<size_t>(_upperBone)];

		_outPole = CUR_U.Translation() + Vector3::TransformNormal(DIR_BIND_LOCAL, CUR_U);
		return true;
	}

	void Animator::RotateGlobalInPlace(Matrix& _global, const Quaternion& _delta)
	{
		const Vector3 pos = _global.Translation();
		_global.Translation(Vector3(0.0f, 0.0f, 0.0f));
		_global = _global * Matrix::CreateFromQuaternion(_delta);
		_global.Translation(pos);
	}

	void Animator::ApplyIKGoals(const Skeleton& _skeleton, const HumanoidBoneMap& _map)
	{
		// 골반 오프셋 - 몸 전체 내리기 발 ik 처리용
		if (m_ikPelvisOffset.LengthSquared() > 1e-12f && _map.Has(HumanoidBone::Hips))
		{
			const int HIPS = _map.Get(HumanoidBone::Hips);
			m_globalPose[static_cast<size_t>(HIPS)].Translation(
				m_globalPose[static_cast<size_t>(HIPS)].Translation() + m_ikPelvisOffset
			);

			// 하위 본 재연산
			_skeleton.RecomputeGlobalPoseFrom(m_localPose, HIPS + 1, m_globalPose);
		}

		// 1패스: 유효한 goal 수집.
		// 사지 처리 순서는 upper 본 인덱스 오름차순이어야 한다 —
		// RecomputeGlobalPoseFrom(start) 는 start~배열 끝을 IK 이전 로컬 포즈로 되돌리므로,
		// 인덱스가 큰 사지를 먼저 풀면 뒤에 처리하는 사지가 그 결과를 통째로 지운다.
		// m_mapIKBinding 은 unordered_map 이라 순회 순서가 해시에 달려 있다.
		// (std::map 으로 바꿔도 안 된다 - enum 순서는 L팔->L다리->R팔->R다리 라 인덱스 오름차순이 아니다)
		std::array<IKOrder, static_cast<size_t>(HumanoidBone::Count)> order{};
		size_t count = 0;

		for (const std::pair<const HumanoidBone, IKData>& p : m_mapIKBinding)
		{
			const IKGoal& GOAL = p.second.goal;
			if (GOAL.positionAlpha <= 0.0f && GOAL.rotationAlpha <= 0.0f)
				continue;

			const TwoBoneIKBinding& BINDING = p.second.binding;
			if (!_map.Has(BINDING.upper) || !_map.Has(BINDING.lower) || !_map.Has(BINDING.end))
				continue;

			order[count++] = { _map.Get(BINDING.upper), &p.second };
		}

		std::sort(order.begin(), order.begin() + count,
			[](const IKOrder& _a, const IKOrder& _b) { return _a.upperIdx < _b.upperIdx; });

		// 2패스: end들 goal에 대한 연산
		int maxRotatedIdx = -1; // 지금까지 직접 회전시킨 본의 최대 인덱스
		for (size_t i = 0; i < count; ++i)
		{
			const IKGoal& GOAL = order[i].pData->goal;
			const TwoBoneIKBinding& binding = order[i].pData->binding;

			const int UPPER_IDX = order[i].upperIdx;
			const int LOWER_IDX = _map.Get(binding.lower);
			const int END_IDX = _map.Get(binding.end);

			// 다음 사지의 재연산 시작점(upper+1)이 앞서 회전시킨 본을 덮으면 결과가 지워진다.
			// 리그가 사지 체인을 인덱스 상 뒤섞어 배치한 경우 - 조용히 틀리지 않도록 알린다
			if (UPPER_IDX <= maxRotatedIdx && !m_bWarnedIKChainOrder)
			{
				m_bWarnedIKChainOrder = true;
				MG_LOG_WARN("[Animator] IK chain index overlap: upper {} <= rotated {}. "
					"Earlier limb IK will be overwritten on this rig.", UPPER_IDX, maxRotatedIdx);
			}

			if (GOAL.positionAlpha > 0.0f)
			{
				const Vector3 U = m_globalPose[static_cast<size_t>(UPPER_IDX)].Translation();
				const Vector3 L = m_globalPose[static_cast<size_t>(LOWER_IDX)].Translation();
				const Vector3 E = m_globalPose[static_cast<size_t>(END_IDX)].Translation();

				TwoBoneIKBone bone;
				bone.upperPos = U;
				bone.lowerPos = L;
				bone.endPos = E;

				TwoBoneIKTarget target;
				target.targetPos = GOAL.position;
				target.alpha = GOAL.positionAlpha;

				// 폴 후보를 순서대로 "시도 -> 실패 시 다음" 으로 내려간다.
				// 폴을 미리 고르고 한 번만 풀면, 명시 폴이 축퇴했을 때 폴백으로
				// 내려가지 못하고 조용히 무동작이 된다.
				// 이 형태면 판정 기준이 솔버의 실제 축(target - upper)이 된다.
				Vector3 pole;
				TwoBoneIKResult result;
				bool bSolved = false;

				if (GOAL.bUsePoleTarget) // 1순위: 명시 폴
				{
					target.poleTargetPos = GOAL.poleTarget;
					bSolved = SolveTwoBone(bone, target, result);
				}

				if (!bSolved && FallbackPole(bone, pole)) // 2순위: 현재 굽힘 평면 보존
				{
					target.poleTargetPos = pole;
					bSolved = SolveTwoBone(bone, target, result);
				}

				if (!bSolved && BindPole(_skeleton, UPPER_IDX, LOWER_IDX, END_IDX, pole)) // 3순위: 바인드 유도
				{
					target.poleTargetPos = pole;
					bSolved = SolveTwoBone(bone, target, result);
				}

				if (bSolved)
				{
					RotateGlobalInPlace(m_globalPose[static_cast<size_t>(UPPER_IDX)], result.upperDelta);
					_skeleton.RecomputeGlobalPoseFrom(m_localPose, UPPER_IDX + 1, m_globalPose);

					RotateGlobalInPlace(m_globalPose[static_cast<size_t>(LOWER_IDX)], result.lowerDelta);
					_skeleton.RecomputeGlobalPoseFrom(m_localPose, LOWER_IDX + 1, m_globalPose);

					maxRotatedIdx = max(maxRotatedIdx, LOWER_IDX);
				}
			}

			if (GOAL.rotationAlpha > 0.0f) 
			{
				Matrix& endGlobal = m_globalPose[static_cast<size_t>(END_IDX)];

				const Vector3 POS = endGlobal.Translation();
				const Quaternion CUR = Quaternion::CreateFromRotationMatrix(endGlobal);
				const float ALPHA = std::clamp(GOAL.rotationAlpha, 0.0f, 1.0f);

				// 델타면 현재 포즈 뒤에 이어 붙인다 (row-vector : 현재 -> 델타 순)
				const Quaternion TARGET = GOAL.bRotationIsDelta
					? Quaternion::Concatenate(CUR, GOAL.rotation)
					: GOAL.rotation;

				endGlobal = Matrix::CreateFromQuaternion(Quaternion::Slerp(CUR, TARGET, ALPHA));
				endGlobal.Translation(POS);
				_skeleton.RecomputeGlobalPoseFrom(m_localPose, END_IDX + 1, m_globalPose); // 자식 본들 위치 정리

				maxRotatedIdx = max(maxRotatedIdx, END_IDX);
			}
		}

	}

	void Animator::RefreshIKAny()
	{
		m_bUseIK = m_ikPelvisOffset.LengthSquared() > 1e-12f;
		if (m_bUseIK) 
			return;

		for (const std::pair<const HumanoidBone, IKData>& p : m_mapIKBinding)
		{
			if (p.second.goal.positionAlpha > 0.0f || p.second.goal.rotationAlpha > 0.0f)
			{
				m_bUseIK = true;
				return;
			}
		}
	}

}

