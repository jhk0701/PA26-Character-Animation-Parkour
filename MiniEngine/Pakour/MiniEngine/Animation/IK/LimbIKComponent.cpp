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
			m_pendingTask[i] = nullptr;
	}

	void LimbIKComponent::LateTick(float _dt)
	{
		Component::LateTick(_dt);
		
		if (m_pSkeletal.expired())
			return;
		
		// IK 예약 작업 실행
		ProcessPendingTask();
		PostPendingTask();

		// 발 위치에 따른 골반 위치 보정
		AdjustPelvisOffset();

		// IK 갱신
		UpdateIK();
	}

	void LimbIKComponent::ProcessPendingTask()
	{
		for (uint8_t i = 0; i < (uint8_t)ELimbType::End; ++i)
		{
			if (!IsEnable(m_handles[i]))
				continue;

			IKHandle& handle = m_handles[i];
			if (m_pendingTask[i])
			{
				TaskResult result = m_pendingTask[i]();
				handle.targetPos = result.position;
				handle.targetRot = result.rotation;
				handle.posAlpha = result.posAlpha;
				handle.rotAlpha = result.rotAlpha;
			}
		}
	}

	void LimbIKComponent::PostPendingTask()
	{
		if (m_pendingTask[(uint8_t)ELimbType::LeftLeg])
			m_handles[(uint8_t)ELimbType::LeftLeg].targetPos.y = std::clamp(m_handles[(uint8_t)ELimbType::LeftLeg].targetPos.y, -m_desc.maxFootDrop, m_desc.maxFootRaise);

		if (m_pendingTask[(uint8_t)ELimbType::RightLeg])
			m_handles[(uint8_t)ELimbType::RightLeg].targetPos.y = std::clamp(m_handles[(uint8_t)ELimbType::RightLeg].targetPos.y, -m_desc.maxFootDrop, m_desc.maxFootRaise);
	}

	void LimbIKComponent::AdjustPelvisOffset()
	{
		IKHandle& hLeftLeg = m_handles[(uint8_t)ELimbType::LeftLeg];
		IKHandle& hRightLeg = m_handles[(uint8_t)ELimbType::RightLeg];

		if (!IsEnable(hLeftLeg) && !IsEnable(hRightLeg))
			return;

		m_pelvisOffset = 
			hLeftLeg.targetPos.y < hRightLeg.targetPos.y ? 
			hLeftLeg.targetPos.y - hLeftLeg.originPosW.y : hRightLeg.targetPos.y - hRightLeg.originPosW.y;
		m_pelvisOffset = std::clamp(m_pelvisOffset, -m_desc.maxPelvisDrop, 0.0f);

		// MG_LOG_INFO("[LimbIK] :: pelvis offset : {}", m_pelvisOffset);
		// m_pSkeletal.lock()->SetIKPelvisOffsetWorld(Vector3(0.0f, m_pelvisOffset, 0.0f));
	}

	void LimbIKComponent::UpdateIK()
	{
		std::shared_ptr<SkeletalMeshComponent> pSkeletal = m_pSkeletal.lock();
		std::shared_ptr<Animator> pAnim = pSkeletal->GetAnim().lock();
		std::shared_ptr<SkinnedMesh> pMesh = pSkeletal->GetMesh().lock();

		if (!pAnim || !pMesh)
			return;

		const HumanoidBoneMap& BONE_MAP = pMesh->GetHumanoidBones();
		const std::vector<Matrix>& GLOBAL_POSE = pAnim->GetGlobalPose();

		// MG_LOG_INFO("[LimbIKComp] Left Arm : {}", m_handles[(uint8_t)ELimbType::LeftArm].posAlpha);

		for (uint8_t i = 0; i < (uint8_t)ELimbType::End; ++i)
		{
			if (!IsEnable(m_handles[i]))
				continue;
			else
				MG_LOG_INFO("[LimbIKComp] type : {} is Enable : {:.2f}", i, m_handles[i].posAlpha);

			IKHandle& handle = m_handles[i];

			// MiniEngine::Debug::DrawPoint(handle.targetPos, MiniEngine::DebugColor::YELLOW, 0.05f, MiniEngine::Debug::EMarkerShape::Sphere, 0.01f);
			pSkeletal->SetIKGoalWorld(
				handle.binding,
				handle.targetPos,
				// handle.targetRot,
				handle.posAlpha// ,
				// handle.rotAlpha
			);

			const int LOWER_IDX = BONE_MAP.Get(handle.binding.lower);
			if (LOWER_IDX < 0 || LOWER_IDX >= static_cast<int>(GLOBAL_POSE.size()))
				continue;

			Vector3 localDir = owner.lock()->ConvertToLocalDir(m_desc.poleDir[i]);
			Vector3 pos = GLOBAL_POSE[static_cast<size_t>(LOWER_IDX)].Translation();

			// MiniEngine::Debug::DrawLine(pos, pos + localDir, MiniEngine::DebugColor::YELLOW, 0.01f);
			pos += localDir;

			pSkeletal->SetIKPoleTargetWorld(handle.binding, pos);
		}
	}

}


