#include "pch.h"
#include "Content/Perception/BeamCompareHeightNode.h"
#include "Physics/PhysicsWorld.h"

#include "Content/Character.h"
#include "Content/Perception/PerceptionNodeUtil.h"
#include "Core/Log.h"

using namespace MiniEngine::Physics;

EPerceptionResult BeamCompareHeightNode::InvokeTask(TravelContext& _context, TravelResult& _result)
{
	std::shared_ptr<Character> pChar = PerceptionNodeUtil::ToChar(_context.m_owner);
	IObstacle* pObs = _context.m_pFirstObstacle;
	/*
	SpherecastParam param;
	param.m_dir = pChar->GetRoot()->localTransform.Forward();
	param.m_startPos = _context.m_firstObstacleHitPos;
	param.m_radius = pChar->GetCapsuleRadius();
	param.m_maxDistance = pChar->GetPerceptionConfig().maxObstacleDetectDist;
	RaycastResult result;
	bool bIsHit = _context.m_physics->SphereCast(param, result, ToMask(Layer::ObstacleLedge));
	const Vector3& OBS_POS = pObs->GetTransform().position;
	_context.m_ledge = bIsHit ? result.m_pos.y : OBS_POS.y;
	*/
	_context.m_ledge = pObs->GetNearestLedgeHeight(_context.m_firstObstacleHitPos);

	return EPerceptionResult::Succeess;
}
