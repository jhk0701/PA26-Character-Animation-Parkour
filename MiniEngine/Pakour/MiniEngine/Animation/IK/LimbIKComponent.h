#pragma once
#include "Scene/Component.h"
#include "Asset/IK.h"
#include "Animation/Animator.h"
#include <array>
#include <functional>

namespace MiniEngine 
{
	class SkeletalMeshComponent;

	enum class ELimbType : uint8_t
	{
		LeftArm,
		RigthArm,
		LeftLeg,
		RightLeg,

		End
	};

	struct LimbIKDesc
	{
		float footHeight{ 0.0f }; // 로컬 foot의 기본 높이

		float maxFootRaise{ 0.45f };
		float maxFootDrop{ 0.45f };  // 다리 길이 절반 근처
		float maxPelvisDrop{ 0.45f };

		float maxHandRotateDeg{ 120.0f }; // 손이 돌 수 있는 최대한의 값

		std::array<Vector3, (uint8_t)ELimbType::End> poleDir;
	};

	// 사지를 IK로 제어하는 컴포넌트
	class LimbIKComponent : public Component
	{
	public:
		struct IKHandle
		{
			// bool bEnable{ false };
			float posAlpha{ 0.0f };
			float rotAlpha{ 0.0f };
			TwoBoneIKBinding binding{};
			Vector3 targetPos{ 0.0f, 0.0f, 0.0f };
			Quaternion targetRot{ 0.0f, 0.0f, 0.0f, 1.0f };

			Vector3 originPosW{ 0.0f, 0.0f, 0.0f };
			Quaternion rotationOffset{ 0.0f, 0.0f, 0.0f, 1.0f };
		};

		struct TaskResult 
		{
			Vector3 position;
			Quaternion rotation;
			float posAlpha{ 1.0f };
			float rotAlpha{ 1.0f };
		};

		void LateTick(float _dt) override;
		void Init(const std::shared_ptr<SkeletalMeshComponent>& _pSkeletal, const LimbIKDesc& _desc);

		/*
		void SetEnableIK(ELimbType _type, bool _bEnable) { m_handles[(uint8_t)_type].bEnable = _bEnable; }
		void SetEnableAllIK(bool _bEnable);
		*/
		void SetPositionAlphaIK(ELimbType _type, float _alpha) { m_handles[(uint8_t)_type].posAlpha = _alpha; }
		void SetRotationAlphaIK(ELimbType _type, float _alpha) { m_handles[(uint8_t)_type].rotAlpha = _alpha; }
		void SetAlphaIK(ELimbType _type, float _alpha) { SetPositionAlphaIK(_type, _alpha); SetRotationAlphaIK(_type, _alpha); }
		void SetTargetPosIK(ELimbType _type, const Vector3& _targetPos) { m_handles[(uint8_t)_type].targetPos = _targetPos; }
		void SetTargetRotIK(ELimbType _type, const Quaternion& _targetRot) { m_handles[(uint8_t)_type].targetRot = _targetRot; }
		void SetOriginPosIK(ELimbType _type, const Vector3& _originPos) { m_handles[(uint8_t)_type].originPosW = _originPos; }

		void SetPendingTask(ELimbType _type, std::function<TaskResult()>&& _task) { m_pendingTask[(uint8_t)_type] = _task; }
		
		void ClearPendingTask();

		const TwoBoneIKBinding& GetBinding(ELimbType _type) const { return m_handles[(uint8_t)_type].binding; }

	private:
		bool IsEnable(const IKHandle& _h) { return _h.posAlpha > 1e-4f || _h.rotAlpha > 1e-4f; }

		void ProcessPendingTask();
		void PostPendingTask();
		void AdjustPelvisOffset();
		void UpdateIK();

		std::weak_ptr<SkeletalMeshComponent> m_pSkeletal;

		std::array<IKHandle, (uint8_t)ELimbType::End> m_handles; // 사지 IK 핸들
		std::array<std::function<TaskResult()>, (uint8_t)ELimbType::End> m_pendingTask; // IK LateUpdate 직전에 행할 태스크, 주로 위치 탐지를 위함

		float m_pelvisOffset{ 0.0f }; // 발 위치에 따라 이동된 골반 - 0.0f 기본값
		LimbIKDesc m_desc;
	};
}