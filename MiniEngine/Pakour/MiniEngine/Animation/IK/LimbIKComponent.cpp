#include "pch.h"
#include "Animation/IK/LimbIKComponent.h"
#include "Scene/SkeletalMeshComponent.h"

namespace MiniEngine 
{

	void LimbIKComponent::Init(const std::shared_ptr<SkeletalMeshComponent>& _pSkeletal)
	{
		m_pSkeletal = _pSkeletal;

		IKHandle& hLeftArm = m_handles[ELimbType::LeftArm];
		hLeftArm.binding.upper = HumanoidBone::LeftUpperArm;
		hLeftArm.binding.lower = HumanoidBone::LeftLowerArm;
		hLeftArm.binding.end = HumanoidBone::LeftHand;

		IKHandle& hRightArm = m_handles[ELimbType::RigthArm];
		hRightArm.binding.upper = HumanoidBone::RightUpperArm;
		hRightArm.binding.lower = HumanoidBone::RightLowerArm;
		hRightArm.binding.end = HumanoidBone::RightHand;

		IKHandle& hLeftLeg = m_handles[ELimbType::LeftLeg];
		hLeftLeg.binding.upper = HumanoidBone::LeftUpperLeg;
		hLeftLeg.binding.lower = HumanoidBone::LeftLowerLeg;
		hLeftLeg.binding.end = HumanoidBone::LeftFoot;

		IKHandle& hRightLeg = m_handles[ELimbType::RightLeg];
		hRightLeg.binding.upper = HumanoidBone::RightUpperLeg;
		hRightLeg.binding.lower = HumanoidBone::RightLowerLeg;
		hRightLeg.binding.end = HumanoidBone::RightFoot;
	}

	void LimbIKComponent::LateTick(float _dt)
	{
		Component::LateTick(_dt);
		
		if (m_pSkeletal.expired())
			return;

		std::shared_ptr<SkeletalMeshComponent> pSkeletal = m_pSkeletal.lock();
		
		// ik 갱신
		for (const IKHandle& handle : m_handles)
		{
			if (!handle.bEnable)
				continue;

			pSkeletal->SetIKGoalWorld(
				handle.binding,
				handle.targetPos,
				handle.targetRot,
				handle.alpha,
				handle.alpha
			);
		}
	}

}


