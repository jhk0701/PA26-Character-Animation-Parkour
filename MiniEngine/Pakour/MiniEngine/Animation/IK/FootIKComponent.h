#pragma once
#include "Scene/Component.h"
#include "Core/Math.h"

namespace MiniEngine::Physics { class PhysicsWorld; }
namespace MiniEngine 
{
	class SkeletalMeshComponent;
	class CharacterControllerComponent;

	struct FootIKDesc 
	{
		float footHeight = 0.0f;

		// 발 기준 레이캐스트 거리
		float traceUpDistance = 0.50f;
		float traceDownDistance = 0.50f;

		// 발, 골반이 애니메이션을 벗어날 수 있는 한계치
		float maxFootRaise = 0.0f;
		float maxFootDrop = 0.45f;
		float maxPelvisDrop = 0.0f;

		float offsetSmoothK = 15.0f;
		float pelvisSmoothK = 10.0f;
		float fadeSmoothK = 10.0f;
		float normalSmoothK = 10.0f;

		float rotationStrength = 1.0f;
		float maxSlopeDeg = 45.0f;

		float liftFadeStart = 0.3f;
		float liftFadeEnd = 0.12f;
	};

	class FootIKComponent : public Component
	{
	public:
		void LateTick(float _dt) override;

		void Init(Physics::PhysicsWorld& _world,
			const std::shared_ptr<SkeletalMeshComponent>& _skeletal,
			const std::shared_ptr<CharacterControllerComponent>& _cct,
			const FootIKDesc& _desc);

		void SetEnabled(bool _bEnabled) { m_bEnable = _bEnabled; }
		bool IsEnabled() const { return m_bEnable; }

	private:
		Physics::PhysicsWorld* m_pPhysWorld{ nullptr };
		std::weak_ptr<SkeletalMeshComponent> m_pSkeletal;
		std::weak_ptr<CharacterControllerComponent> m_pCCT;
		FootIKDesc m_desc;

		bool m_bEnable{ false };
		float m_alpha{ 0.0f };
		float m_pelvisOffset{ 0.0f };

		bool m_bInitialized{ false };
		bool m_bGoalActive{ false };
		bool m_bFootHeightResolved{ false };
	};
}