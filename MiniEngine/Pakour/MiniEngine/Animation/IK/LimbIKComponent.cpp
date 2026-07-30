#include "pch.h"
#include "Animation/IK/LimbIKComponent.h"
#include "Scene/Actor.h"
#include "Scene/SkeletalMeshComponent.h"
#include "Animation/Animator.h"
#include "Core/Log.h"
#include "Core/DebugMarkers.h"

namespace MiniEngine 
{

	void LimbIKComponent::Init(const std::shared_ptr<SkeletalMeshComponent>& _pSkeletal, const LimbIKDesc& _desc)
	{
		m_pSkeletal = _pSkeletal;
		m_desc = _desc;
		for (Vector3& poleDir : m_desc.poleDir)
			poleDir.Normalize();

		m_pelvisOffset = 0.0f;

		IKHandle& hLeftArm = m_handles[(uint8_t)ELimbType::LeftArm];
		hLeftArm.binding.upper = HumanoidBone::LeftUpperArm;
		hLeftArm.binding.lower = HumanoidBone::LeftLowerArm;
		hLeftArm.binding.end = HumanoidBone::LeftHand;

		IKHandle& hRightArm = m_handles[(uint8_t)ELimbType::RigthArm];
		hRightArm.binding.upper = HumanoidBone::RightUpperArm;
		hRightArm.binding.lower = HumanoidBone::RightLowerArm;
		hRightArm.binding.end = HumanoidBone::RightHand;

		IKHandle& hLeftLeg = m_handles[(uint8_t)ELimbType::LeftLeg];
		hLeftLeg.binding.upper = HumanoidBone::LeftUpperLeg;
		hLeftLeg.binding.lower = HumanoidBone::LeftLowerLeg;
		hLeftLeg.binding.end = HumanoidBone::LeftFoot;

		IKHandle& hRightLeg = m_handles[(uint8_t)ELimbType::RightLeg];
		hRightLeg.binding.upper = HumanoidBone::RightUpperLeg;
		hRightLeg.binding.lower = HumanoidBone::RightLowerLeg;
		hRightLeg.binding.end = HumanoidBone::RightFoot;
	}

	//void LimbIKComponent::SetEnableAllIK(bool _bEnable)
	//{
	//	for (uint8_t i = 0; i < (uint8_t)ELimbType::End; ++i)
	//		m_handles[i].bEnable = _bEnable;
	//}

	void LimbIKComponent::ClearPendingTask()
	{
		for (uint8_t i = 0; i < (uint8_t)ELimbType::End; ++i)
		{
			if (!m_pendingTask[i])
				continue;

			m_pendingTask[i] = nullptr;

			// 예약이 걷혔으면 해당 사지는 페이드아웃시킨다 - alpha 가 그대로 남지 않도록
			m_handles[i].posAlphaTarget = 0.0f;
			m_handles[i].rotAlphaTarget = 0.0f;
			m_handles[i].bGroundValid = false;
		}
	}

	void LimbIKComponent::LateTick(float _dt)
	{
		Component::LateTick(_dt);

		if (m_pSkeletal.expired())
			return;

		// IK 예약 작업 실행
		ProcessPendingTask();
		FadeAlpha(_dt);
		PostPendingTask();

		// 발 위치에 따른 골반 위치 보정
		AdjustPelvisOffset(_dt);

		// IK 갱신
		UpdateIK();
	}

	void LimbIKComponent::ProcessPendingTask()
	{
		// alpha 로 게이트하지 않는다.
		// 예약 작업이 alpha 를 만들어내는 주체이므로, alpha 가 0 이라고 건너뛰면
		// 작업이 영원히 실행되지 않는 교착이 된다.
		for (uint8_t i = 0; i < (uint8_t)ELimbType::End; ++i)
		{
			if (!m_pendingTask[i])
				continue;

			IKHandle& handle = m_handles[i];
			const TaskResult RESULT = m_pendingTask[i]();

			handle.bGroundValid = (RESULT.posAlpha > 0.0f);

			// alpha 0 인 결과는 타깃을 덮어쓰지 않는다.
			// 조기 반환한 TaskResult 의 position/rotation 은 기본값이라,
			// 그대로 받으면 페이드아웃 중에 발이 원점으로 끌려간다.
			if (RESULT.posAlpha > 0.0f)
				handle.targetPos = RESULT.position;

			if (RESULT.rotAlpha > 0.0f)
				handle.targetRot = RESULT.rotation;

			handle.posAlphaTarget = RESULT.posAlpha;
			handle.rotAlphaTarget = RESULT.rotAlpha;
		}
	}

	void LimbIKComponent::FadeAlpha(float _dt)
	{
		const float STEP = m_desc.alphaFadeSpeed * _dt;

		for (uint8_t i = 0; i < (uint8_t)ELimbType::End; ++i)
		{
			IKHandle& handle = m_handles[i];
			handle.posAlpha += std::clamp(handle.posAlphaTarget - handle.posAlpha, -STEP, STEP);
			handle.rotAlpha += std::clamp(handle.rotAlphaTarget - handle.rotAlpha, -STEP, STEP);
		}
	}

	void LimbIKComponent::PostPendingTask()
	{
		// 다리 후처리
		const ELimbType LEGS[2] = { ELimbType::LeftLeg, ELimbType::RightLeg };

		for (const ELimbType& TYPE : LEGS)
		{
			IKHandle& handle = m_handles[(uint8_t)TYPE];
			if (!handle.bGroundValid)
				continue;

			// 발 본은 발목이라, 히트점에 그대로 놓으면 발이 지면에 파묻힌다
			handle.targetPos.y += m_desc.footHeight;

			const float DELTA = std::clamp(handle.targetPos.y - handle.originPosW.y,
				-m_desc.maxFootDrop, m_desc.maxFootRaise);

			handle.targetPos.y = handle.originPosW.y + DELTA;
		}
	}

	void LimbIKComponent::AdjustPelvisOffset(float _dt)
	{
		// 더 많이 내려가야 하는 쪽 발의 델타만큼 골반을 내리기
		const ELimbType LEGS[2] = { ELimbType::LeftLeg, ELimbType::RightLeg };

		float target = 0.0f;
		bool bAny = false;

		for (const ELimbType& TYPE : LEGS)
		{
			const IKHandle& HANDLE = m_handles[(uint8_t)TYPE];
			if (!HANDLE.bGroundValid || HANDLE.posAlpha <= 1e-4f)
				continue;

			const float DELTA = (HANDLE.targetPos.y - HANDLE.originPosW.y) * HANDLE.posAlpha;
			target = (bAny && target < DELTA) ? target : DELTA;
			bAny = true;
		}

		target = std::clamp(target, -m_desc.maxPelvisDrop, 0.0f); // 내리기

		const float STEP = m_desc.pelvisLerpSpeed * _dt;
		m_pelvisOffset += std::clamp(target - m_pelvisOffset, -STEP, STEP);

		// MG_LOG_INFO("[LimbIK] :: pelvis offset : {}", m_pelvisOffset);
		m_pSkeletal.lock()->SetIKPelvisOffsetWorld(Vector3(0.0f, m_pelvisOffset, 0.0f));
	}

	void LimbIKComponent::UpdateIK()
	{
		std::shared_ptr<SkeletalMeshComponent> pSkeletal = m_pSkeletal.lock();
		std::shared_ptr<Animator> pAnim = pSkeletal->GetAnim().lock();
		std::shared_ptr<SkinnedMesh> pMesh = pSkeletal->GetMesh().lock();

		if (!pAnim || !pMesh)
			return;

		const HumanoidBoneMap& BONE_MAP = pMesh->GetHumanoidBones();

		// MG_LOG_INFO("[LimbIKComp] Left Arm : {}", m_handles[(uint8_t)ELimbType::LeftArm].posAlpha);

		for (uint8_t i = 0; i < (uint8_t)ELimbType::End; ++i)
		{
			if (!IsEnable(m_handles[i]))
				continue;

			IKHandle& handle = m_handles[i];

			// MiniEngine::Debug::DrawPoint(handle.targetPos, MiniEngine::DebugColor::YELLOW, 0.05f, MiniEngine::Debug::EMarkerShape::Sphere, 0.01f);

			pSkeletal->SetIKGoalWorldWithRotDelta(
				handle.binding,
				handle.targetPos,
				handle.posAlpha,
				handle.targetRot,
				handle.rotAlpha
			);

			if (m_desc.poleDir[i].LengthSquared() < 1e-6f)
				continue;

			Matrix upperW;
			if (!pSkeletal->GetBoneWorldMatrix(BONE_MAP.Get(handle.binding.upper), upperW))
				continue;

			Vector3 worldDir = owner.lock()->ConvertToActorDir(m_desc.poleDir[i]);
			Vector3 pos = upperW.Translation();

			MiniEngine::Debug::DrawLine(pos, pos + worldDir, MiniEngine::DebugColor::YELLOW, 0.01f);
			pos += worldDir;

			pSkeletal->SetIKPoleTargetWorld(handle.binding, pos);
		}
	}

}


