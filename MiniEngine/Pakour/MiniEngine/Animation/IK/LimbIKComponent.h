#pragma once
#include "Scene/Component.h"
#include "Asset/IK.h"
#include "Animation/Animator.h"
#include <array>
#include <functional>

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
			bool bEnable{ false };
			float posAlpha{ 1.0f };
			float rotAlpha{ 1.0f };
			TwoBoneIKBinding binding{};
			Vector3 targetPos{ 0.0f, 0.0f, 0.0f };
			Quaternion targetRot{ 0.0f, 0.0f, 0.0f, 1.0f };
		};

		struct TaskResult 
		{
			Vector3 position;
			Quaternion rotation;
			float posAlpha{ 1.0f };
			float rotAlpha{ 1.0f };
		};

		void LateTick(float _dt) override;
		void Init(const std::shared_ptr<SkeletalMeshComponent>& _pSkeletal);

		void SetEnableIK(ELimbType _type, bool _bEnable) { m_handles[_type].bEnable = _bEnable; }
		void SetEnableAllIK(bool _bEnable);
		void SetPositionAlphaIK(ELimbType _type, float _alpha) { m_handles[_type].posAlpha = _alpha; }
		void SetRotationAlphaIK(ELimbType _type, float _alpha) { m_handles[_type].rotAlpha = _alpha; }
		void SetAlphaIK(ELimbType _type, float _alpha) { SetPositionAlphaIK(_type, _alpha); SetRotationAlphaIK(_type, _alpha); }
		void SetTargetPosIK(ELimbType _type, const Vector3& _targetPos) { m_handles[_type].targetPos = _targetPos; }
		void SetTargetRotIK(ELimbType _type, const Quaternion& _targetRot) { m_handles[_type].targetRot = _targetRot; }
		void SetPendingTask(ELimbType _type, std::function<TaskResult()>&& _task) { m_pendingTask[_type] = _task; }
		
		void ClearPendingTask();

		const TwoBoneIKBinding& GetBinding(ELimbType _type) const { return m_handles[_type].binding; }

	private:
		std::weak_ptr<SkeletalMeshComponent> m_pSkeletal;

		std::array<IKHandle, ELimbType::End> m_handles; // 사지 IK 핸들
		std::array<std::function<TaskResult()>, ELimbType::End> m_pendingTask; // IK LateUpdate 직전에 행할 태스크, 주로 위치 탐지를 위함
	};
}