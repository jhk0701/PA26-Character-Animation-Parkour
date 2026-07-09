#pragma once
#include "Scene/Component.h"
#include "Core/Math.h"

namespace physx { class PxRigidActor; }

namespace MiniEngine 
{
	namespace Physics 
	{ 
		enum ECollisionGroup : uint16_t;
		class PhysicsWorld; 
	}

	class RigidBodyComponent : public Component
	{
	public:
		enum class EBodyType { Static, Dynamic };

		void Init(Physics::PhysicsWorld& _world, EBodyType _type, const Vector3& _halfExtents, float _denity = 10.0f);
		void InitDynamicCapsule(Physics::PhysicsWorld& _world, const Vector2& _capsuleExtent, float _denity = 10.0f);
		
		EBodyType GetBodyType() const { return m_type; }
		
		void SetCollsionGroup(Physics::ECollisionGroup _group);
		Physics::ECollisionGroup GetCollsionGroup() const;

		void SetKinemetic(bool _bIsOn);

		// TODO : 빠른 테스트용 임시 코드 -> 제거하고 위계대로 적용시킬 것
		void SyncTransform();

	private:
		physx::PxRigidActor* m_actor = nullptr;
		EBodyType m_type = EBodyType::Static;
	};
}
