#include "pch.h"
#include "Content/Perception/CheckObstacleOnHangingNode.h"
#include "Content/Perception/PerceptionNodeUtil.h"
#include "Content/Character.h"

#include "Physics/PhysicsWorld.h"

using namespace MiniEngine;
using namespace MiniEngine::Physics;
using namespace PerceptionNodeUtil;

bool CheckOnHangingMoveUpNode::InvokeCondition(TravelContext& _context)
{
	std::shared_ptr<Character> pChar = ToChar(_context.m_owner);
	const PerceptionConfig& CONFIG = pChar->GetPerceptionConfig();

	SpherecastParam param;
	param.m_startPos = GetCharacterCenterPosition(_context);
	param.m_dir = Vector3(0.0f, 1.0f, 0.0f);
	param.m_radius = CONFIG.onHangingSearchRadius;
	param.m_maxDistance = CONFIG.onHangingSearchDist;

	RaycastResult result;
	if (_context.m_physics->SphereCast(param, result, ToMask(Layer::ObstacleLedge)) == false)
		return false;

	FillFromResult(_context, result);
	return true;
}

bool CheckOnHangingMoveDownNode::InvokeCondition(TravelContext& _context)
{
	std::shared_ptr<Character> pChar = ToChar(_context.m_owner);

	RaycastParam param;
	param.m_origin = pChar->GetRoot()->localTransform.position;
	param.m_dir = Vector3(0.0f, -1.0f, 0.0f);
	param.m_maxDistance = pChar->GetPerceptionConfig().onHangingSearchDist;

	RaycastResult result;
	if (_context.m_physics->Raycast(param, result, Layer::Obstacle | Layer::Ground) == false)
		return false;

	FillFromResult(_context, result);
	return true;
}

bool CheckOnHangingMoveSideNode::InvokeCondition(TravelContext& _context)
{
	std::shared_ptr<Character> pChar = ToChar(_context.m_owner);
	const Transform& TF = pChar->GetRoot()->localTransform;

	RaycastParam param;
	param.m_origin = pChar->GetRoot()->localTransform.position;
	param.m_dir = pChar->GetInputDir().x > 0.0f ? TF.Right() : -TF.Right();
	param.m_maxDistance = pChar->GetPerceptionConfig().onHangingSearchDist;

	RaycastResult result;
	if (_context.m_physics->Raycast(param, result, ToMask(Layer::Obstacle)) == false)
		return false;

	FillFromResult(_context, result);
	return true;
}

