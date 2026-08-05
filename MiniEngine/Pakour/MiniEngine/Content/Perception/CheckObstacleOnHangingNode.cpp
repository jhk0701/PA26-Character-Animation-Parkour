#include "pch.h"
#include "Content/Perception/CheckObstacleOnHangingNode.h"
#include "Content/Perception/PerceptionNodeUtil.h"
#include "Content/Character.h"
#include "Content/Data/CharacterPerceptionConfig.h"
#include "Core/DebugMarkers.h"
#include "Core/Log.h"

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
	param.m_maxDistance = CONFIG.onHangingSearchDist;
	param.m_radius = CONFIG.onHangingSearchRadius;

	RaycastResult result;
	if (_context.m_physics->SphereCast(param, result, ToMask(Layer::Obstacle)) == false)
		return false;

	FillFromResult(_context, result);
	return true; // 윗면에 장애물 확인
	// true : 위에 장애물이 있음. 지붕같은 경우 -> 끝단을 찾기
	// false : 위에 장애물은 없음. 현재 장애물의 끝단 찾기
}

// 윗면에 장애물이 없는 상황 -> Ledge 찾기
bool CheckOnHangingUpwardLedgeNode::InvokeCondition(TravelContext& _context)
{
	std::shared_ptr<Character> pChar = ToChar(_context.m_owner);
	const Transform& TF = pChar->GetRoot()->localTransform;
	const PerceptionConfig& CONFIG = pChar->GetPerceptionConfig();

	Vector3 startOffset(0.0f);
	startOffset += TF.Right() * m_startOffset.x;
	startOffset += TF.Up() * m_startOffset.y;
	startOffset += TF.Forward() * m_startOffset.z;

	SpherecastParam param;
	param.m_startPos = GetCharacterCenterPosition(_context) + startOffset; 
	param.m_dir = Vector3(0.0f, 1.0f, 0.0f);
	param.m_radius = CONFIG.onHangingSearchRadius;
	param.m_maxDistance = CONFIG.onHangingSearchDist;

	// MiniEngine::Debug::DrawLine(param.m_startPos, param.m_startPos + param.m_dir * param.m_maxDistance, MiniEngine::DebugColor::YELLOW, 0.1f);
	// MiniEngine::Debug::DrawPoint(param.m_startPos + param.m_dir * param.m_maxDistance, MiniEngine::DebugColor::YELLOW, param.m_radius, MiniEngine::Debug::EMarkerShape::Sphere, 0.1f);

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
	const PerceptionConfig& CONFIG = pChar->GetPerceptionConfig();

	Vector3 startOffset(0.0f);
	startOffset += TF.Right() * m_startOffset.x;
	startOffset += TF.Up() * m_startOffset.y;
	startOffset += TF.Forward() * m_startOffset.z; // 0.3f이 적정

	SpherecastParam param;
	param.m_startPos = GetCharacterCenterPosition(_context) + startOffset;
	param.m_dir = pChar->GetInputDir().x > 0.0f ? TF.Right() : -TF.Right();
	param.m_maxDistance = CONFIG.onHangingSearchDist;
	param.m_radius = CONFIG.onHangingSearchRadius;

	// MiniEngine::Debug::DrawLine(param.m_startPos, param.m_startPos + param.m_dir * param.m_maxDistance, MiniEngine::DebugColor::YELLOW, 0.1f);
	// MiniEngine::Debug::DrawPoint(param.m_startPos + param.m_dir * param.m_maxDistance, MiniEngine::DebugColor::YELLOW, param.m_radius, MiniEngine::Debug::EMarkerShape::Sphere, 0.1f);

	RaycastResult result;
	if (_context.m_physics->SphereCast(param, result, ToMask(Layer::Obstacle)) == false)
		return false;

	FillFromResult(_context, result);

	// 접촉 확인
	// 높이 측정
	const uint8_t BAND = PerceptionNodeUtil::MeasureObstacleHeight(_context, param.m_dir);
	// MG_LOG_INFO("[CheckOnHangingMoveSideNode] Measure Height");
	
	// 매달리는 상황은 아닌 경우
	if (BAND < CONFIG.maxHeightStep)
	{
		PerceptionNodeUtil::MeasureObstacleDepth(_context, param.m_dir); // 깊이 측정
		// MG_LOG_INFO("[CheckOnHangingMoveSideNode] Measure Depth");
	}

	return true;
}