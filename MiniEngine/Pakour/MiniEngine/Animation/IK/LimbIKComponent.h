#pragma once
#include "Scene/Component.h"
#include "Asset/IK.h"
#include "Animation/Animator.h"
#include <array>

namespace MiniEngine 
{
	class SkeletalMeshComponent;


	// 사지를 IK로 제어하는 컴포넌트
	class LimbIKComponent : public Component
	{
	public:
		enum ELimbType : uint8_t
		{
			LeftArm,
			RigthArm,
			LeftLeg,
			RightLeg,

			End
		};

		struct IKHandle
		{
			TwoBoneIKBinding binding;
			float alpha{ 1.0f };
			Vector3 targetPos{ 0.0f, 0.0f, 0.0f };
		};

		void LateTick(float _dt) override;
		void Init(const std::shared_ptr<SkeletalMeshComponent>& _pSkeletal);

	private:
		std::weak_ptr<SkeletalMeshComponent> m_pSkeletal;
		std::array<IKHandle, ELimbType::End> m_handles;

	};
}