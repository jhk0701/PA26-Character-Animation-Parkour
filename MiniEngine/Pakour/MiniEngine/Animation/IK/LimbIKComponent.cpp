#include "pch.h"
#include "Animation/IK/LimbIKComponent.h"

namespace MiniEngine 
{

	void LimbIKComponent::Init(const std::shared_ptr<SkeletalMeshComponent>& _pSkeletal)
	{
		m_pSkeletal = _pSkeletal;

		IKHandle& hLeftArm = m_handles[ELimbType::LeftArm];
		hLeftArm.binding.upper = HumanoidBone::LeftUpperArm;
		hLeftArm.binding.lower = HumanoidBone::LeftLowerArm;
		hLeftArm.binding.end = HumanoidBone::LeftHand;
		hLeftArm.alpha = 0.0f;

		IKHandle& hRightArm = m_handles[ELimbType::RigthArm];
		hRightArm.binding.upper = HumanoidBone::RightUpperArm;
		hRightArm.binding.lower = HumanoidBone::RightLowerArm;
		hRightArm.binding.end = HumanoidBone::RightHand;
		hRightArm.alpha = 0.0f;

		IKHandle& hLeftLeg = m_handles[ELimbType::LeftLeg];
		hLeftLeg.binding.upper = HumanoidBone::LeftUpperLeg;
		hLeftLeg.binding.lower = HumanoidBone::LeftLowerLeg;
		hLeftLeg.binding.end = HumanoidBone::LeftFoot;
		hLeftLeg.alpha = 0.0f;

		IKHandle& hRightLeg = m_handles[ELimbType::RightLeg];
		hRightLeg.binding.upper = HumanoidBone::RightUpperLeg;
		hRightLeg.binding.lower = HumanoidBone::RightLowerLeg;
		hRightLeg.binding.end = HumanoidBone::RightFoot;
		hRightLeg.alpha = 0.0f;
	}

	void LimbIKComponent::LateTick(float _dt)
	{
		Component::LateTick(_dt);


	}

}


