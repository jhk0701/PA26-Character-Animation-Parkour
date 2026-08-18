#include "pch.h"
#include "Perception/Node/MeasureObstacleNode.h"
#include "Perception/Interface/IPerceptionProcessor.h"
#include "Perception/Node/PerceptionNodeUtil.h"
#include "Scene/Actor.h"

#include "Physics/PhysicsWorld.h"
#include "Core/DebugMarkers.h"

using namespace MiniEngine::Physics;

EPerceptionResult MeasureObstacleHeightNode::InvokeTask(TravelContext& _context, TravelResult& _result)
{
	std::shared_ptr<IPerceptionProcessor> pProcessor = std::dynamic_pointer_cast<IPerceptionProcessor>(_context.m_owner);
	if (!pProcessor)
		return EPerceptionResult::Fail;

	const Transform& TF = _context.m_owner->GetRoot()->localTransform;
	const float FOOT_Y = TF.position.y;

	const PerceptionConfig& CONFIG = pProcessor->GetPerceptionConfig();
	const Vector3& PROBE_XZ = _context.m_firstObstacleHitPos;
	
	Vector3 dir(0.0f);
	PerceptionNodeUtil::LocalizeDirection(TF, GetDirection(), dir);

	uint8_t band = 0;
	bool bFirstTouched = false;

	for (; band < CONFIG.maxHeightStep; ++band)
	{
		SpherecastParam param;
		param.m_startPos = Vector3(PROBE_XZ.x,
			FOOT_Y + CONFIG.heightLift + CONFIG.heightRadius + band * CONFIG.heightStep,
			PROBE_XZ.z
		);

		param.m_radius = CONFIG.heightRadius;
		param.m_dir = dir;
		param.m_maxDistance = CONFIG.heightSearchtDist;

		RaycastResult result;
		if (_context.m_physics->SphereCast(param, result, ToMask(Layer::Obstacle)) == false)
		{
#if MG_DEBUG_LOG
			MiniEngine::Debug::DrawPoint(param.m_startPos, MiniEngine::DebugColor::RED, param.m_radius, MiniEngine::Debug::EMarkerShape::Sphere, 1.0f);
#endif // MG_DEBUG_LOG
			// 최소 한번 닿았음 근데, hit가 끊어짐 -> 최고점 도달 처리
			if (bFirstTouched)
				break; // 닿지 않음
		}
		else
		{
#if MG_DEBUG_LOG
			MiniEngine::Debug::DrawPoint(param.m_startPos, MiniEngine::DebugColor::GREEN, param.m_radius, MiniEngine::Debug::EMarkerShape::Sphere, 1.0f);
#endif // MG_DEBUG_LOG
			if (!bFirstTouched)
				bFirstTouched = true;
		}
	}

	_context.m_ledge = bFirstTouched ? 
		FOOT_Y + band * CONFIG.heightStep : 
		FOOT_Y;

	if (band > 0 && band < CONFIG.maxHeightStep)
	{
		SpherecastParam param;
		param.m_startPos = Vector3(
			PROBE_XZ.x, 
			_context.m_ledge - CONFIG.heightRadius, 
			PROBE_XZ.z);
		param.m_dir = dir;
		param.m_radius = CONFIG.ledgeDetectRadius;
		param.m_maxDistance = CONFIG.minObstacleDetectDist;

#if MG_DEBUG_LOG
		MiniEngine::Debug::DrawPoint(param.m_startPos, MiniEngine::DebugColor::BLUE, param.m_radius, MiniEngine::Debug::EMarkerShape::Sphere, 1.0f);
#endif // MG_DEBUG_LOG

		RaycastResult ledgeResult;
		if (_context.m_physics->SphereCast(param, ledgeResult, ToMask(Layer::ObstacleLedge))) 
		{
			_context.m_ledge = ledgeResult.m_pos.y;
			_context.m_bDetectLedge = true;
		}
	}

	return EPerceptionResult::Succeess;
}

EPerceptionResult MeasureObstacleDepthNode::InvokeTask(TravelContext& _context, TravelResult& _result)
{
	std::shared_ptr<IPerceptionProcessor> pProcessor = std::dynamic_pointer_cast<IPerceptionProcessor>(_context.m_owner);
	if (!pProcessor)
		return EPerceptionResult::Fail;

	const PerceptionConfig& CONFIG = pProcessor->GetPerceptionConfig();
	const Vector3 TOP(
		_context.m_firstObstacleHitPos.x,
		_context.m_ledge + CONFIG.depthLift,
		_context.m_firstObstacleHitPos.z
	);
	
	Vector3 dir(0.0f);
	PerceptionNodeUtil::LocalizeDirection(_context.m_owner->GetRoot()->localTransform, GetDirection(), dir);

	float depth = 0.0f;
	for (uint8_t i = 1; i <= CONFIG.maxDepthStep; ++i)
	{
		RaycastParam param;
		param.m_origin = TOP + dir * (CONFIG.depthStep * i);
		param.m_dir = Vector3(0.0f, -1.0f, 0.0f);
		param.m_maxDistance = CONFIG.depthSearchDownDist;

#if MG_DEBUG_LOG
		MiniEngine::Debug::DrawLine(param.m_origin, param.m_origin + param.m_dir * param.m_maxDistance, MiniEngine::DebugColor::GREEN, 1.0f);
#endif // MG_DEBUG_LOG

		RaycastResult result;
		if (_context.m_physics->Raycast(param, result, ToMask(Layer::Obstacle)) == false)
			break; // 못 딛는 구간

		if (PerceptionNodeUtil::ToIObstacle(result.GetActor()) != _context.m_pFirstObstacle)
			break;

		depth = CONFIG.depthStep * i;
	}

	_context.m_depth = depth;

	return EPerceptionResult::Succeess;
}
