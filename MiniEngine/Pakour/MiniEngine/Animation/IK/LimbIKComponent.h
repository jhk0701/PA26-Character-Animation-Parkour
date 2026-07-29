#pragma once
#include "Scene/Component.h"
#include "Asset/IK.h"
#include "Animation/Animator.h"
#include <array>

namespace MiniEngine 
{
	class SkeletalMeshComponent;

	namespace Physics { class PhysicsWorld; }

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
			bool bEnable{ false };
			float alpha{ 1.0f };
			TwoBoneIKBinding binding{};
			Vector3 targetPos{ 0.0f, 0.0f, 0.0f };
			Quaternion targetRot{ 0.0f, 0.0f, 0.0f, 1.0f };
		};

		void LateTick(float _dt) override;
		void Init(const std::shared_ptr<SkeletalMeshComponent>& _pSkeletal);

		void SetEnableIK(ELimbType _type, bool _bEnable) { m_handles[_type].bEnable = _bEnable; }
		void SetAlphaIK(ELimbType _type, float _alpha) { m_handles[_type].alpha = _alpha; }
		void SetTargetPosIK(ELimbType _type, const Vector3& _targetPos) { m_handles[_type].targetPos = _targetPos; }
		void SetTargetRotIK(ELimbType _type, const Quaternion& _targetRot) { m_handles[_type].targetRot = _targetRot; }

	private:
		std::weak_ptr<SkeletalMeshComponent> m_pSkeletal;
		std::array<IKHandle, ELimbType::End> m_handles;
	};
}