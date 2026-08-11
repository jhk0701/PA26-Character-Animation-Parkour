#include "pch.h"
#include "Content/TransitionState.h"
#include "Content/Character.h"

#include "Content/Data/CharacterConfigData.h"
#include "Physics/PhysicsWorld.h"
#include "Scene/Scene.h"

#include "Core/Log.h"
#include "Core/DebugMarkers.h"

void TransitionState::Activate(MiniEngine::AnimNotifyParam& _param)
{
	if (!_param.m_pActor)
		return;

	Character* pChar = dynamic_cast<Character*>(_param.m_pActor);
	pChar->TransitionStateMachine(m_targetState);
}

void CheckIsFallingNotify::Activate(MiniEngine::AnimNotifyParam& _param)
{
	Character* pChar = dynamic_cast<Character*>(_param.m_pActor);
	if (!pChar)
		return;

	std::shared_ptr<MiniEngine::Physics::PhysicsWorld> pPhysics = pChar->GetScene()->GetPhysics().lock();
	
	MiniEngine::Physics::RaycastParam param;
	param.m_dir = Vector3(0.0f, -1.0f, 0.0f);
	param.m_maxDistance = pChar->GetConfig().onLandingFallingCheckDist;
	param.m_origin = pChar->GetRoot()->localTransform.position;

	// MG_LOG_INFO("[CheckIsFallingNotify] :: check falling");
	// MiniEngine::Debug::DrawLine(param.m_origin, param.m_origin + param.m_dir * param.m_maxDistance, MiniEngine::DebugColor::MAGENTA, 1.0f);

	MiniEngine::Physics::RaycastResult result;
	if (pPhysics->Raycast(param, result, MiniEngine::Physics::Layer::Ground | MiniEngine::Physics::Layer::Obstacle) == false)
	{
		pChar->TransitionStateMachine((uint8_t)Character::EState::InAir);
		// MG_LOG_INFO("[CheckIsFallingNotify] :: is falling");
	}

}
