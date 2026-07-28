#include "pch.h"
#include "FootIKComponent.h"

namespace MiniEngine 
{
	void FootIKComponent::Init(Physics::PhysicsWorld& _world, 
		const std::shared_ptr<SkeletalMeshComponent>& _skeletal, 
		const std::shared_ptr<CharacterControllerComponent>& _cct, 
		const FootIKDesc& _desc)
	{
		m_pPhysWorld = &_world;
		m_pSkeletal = _skeletal;
		m_pCCT = _cct;
		m_desc = _desc;
		m_bFootHeightResolved = m_desc.footHeight > 0.0f;
	}

	void FootIKComponent::LateTick(float _dt)
	{
		Component::LateTick(_dt);
	}


}


