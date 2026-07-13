#pragma once
#include "Scene/Component.h"
#include "Core/Math.h"
#include "Physics/CollsionLayer.h"

namespace physx { class PxRigidActor; }

namespace MiniEngine 
{
	class SceneComponent;

	namespace Physics 
	{ 
		enum ECollisionGroup : uint16_t;
		class PhysicsWorld; 
	}

	class RigidBodyComponent : public Component
	{
	public:
		enum class EBodyType { Static, Dynamic };

		void FixedTick(float _dt) override;

		void Init(Physics::PhysicsWorld& _world, EBodyType _type, 
			const Vector3& _halfExtents, float _denity = 10.0f,
			const std::shared_ptr<SceneComponent>& _target = nullptr);
		
		std::shared_ptr<SceneComponent> GetTarget() const { return m_target.lock(); }
		EBodyType GetBodyType() const { return m_type; }
		
		/*void SetCollsionGroup(Physics::ECollisionGroup _group);
		Physics::ECollisionGroup GetCollsionGroup() const;*/

		void SyncTransform(); // PhysX -> Actor Transform
		void PushTransform(); // Actor Transform -> PhysX

		void SetQueryLayer(Physics::Layer _layer);
		void SetCollisionLayer(Physics::Layer _layer);

	private:
		void CheckParented(const std::shared_ptr<SceneComponent>& _target);

		physx::PxRigidActor* m_actor = nullptr;
		std::weak_ptr<SceneComponent> m_target;
		EBodyType m_type = EBodyType::Static;
	};
}
