#include "pch.h"
#include "Perception/Node/MeasureObstacleNode.h"
#include "Perception/Interface/IPerceptionProcessor.h"
#include "Perception/Node/PerceptionNodeUtil.h"
#include "Scene/Actor.h"

#include "Physics/PhysicsWorld.h"
#include "Core/DebugMarkers.h"
#include "Core/Log.h"

using namespace MiniEngine::Physics;

EPerceptionResult MeasureObstacleHeightNode::InvokeTask(TravelContext& _context, PerceptResult& _result)
{
	std::shared_ptr<IPerceptionProcessor> pProcessor = std::dynamic_pointer_cast<IPerceptionProcessor>(_context.owner);
	if (!pProcessor)
		return EPerceptionResult::Fail;

	const Transform& TF = _context.owner->GetRoot()->localTransform;
	const float FOOT_Y = TF.position.y;

	const PerceptionConfig& CONFIG = pProcessor->GetPerceptionConfig();
	const Vector3& PROBE_XZ = _context.intermediate.obstacleHitPos;
	
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
		if (_context.physics->SphereCast(param, result, ToMask(Layer::Obstacle)) == false)
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

	_context.intermediate.obstacleLedge = bFirstTouched ? 
		FOOT_Y + CONFIG.heightLift + band * CONFIG.heightStep :
		FOOT_Y + CONFIG.heightLift;

	if (band > 0 && band <= CONFIG.maxHeightStep)
	{
		SpherecastParam param;
		param.m_startPos = Vector3(
			PROBE_XZ.x,
			_context.intermediate.obstacleLedge - CONFIG.heightRadius,
			PROBE_XZ.z);
		param.m_dir = dir;
		param.m_radius = CONFIG.ledgeDetectRadius;
		param.m_maxDistance = CONFIG.minObstacleDetectDist;

#if MG_DEBUG_LOG
		MiniEngine::Debug::DrawPoint(param.m_startPos, MiniEngine::DebugColor::BLUE, param.m_radius * 1.1f, MiniEngine::Debug::EMarkerShape::Sphere, 1.0f);
#endif // MG_DEBUG_LOG

		RaycastResult ledgeResult;
		if (_context.physics->SphereCast(param, ledgeResult, ToMask(Layer::ObstacleLedge)))
		{
			_context.intermediate.obstacleLedge = ledgeResult.m_pos.y;
			_context.intermediate.bDetectLedge = true;
		}
	}

	return EPerceptionResult::Succeess;
}

EPerceptionResult MeasureObstacleDepthNode::InvokeTask(TravelContext& _context, PerceptResult& _result)
{
	std::shared_ptr<IPerceptionProcessor> pProcessor = std::dynamic_pointer_cast<IPerceptionProcessor>(_context.owner);
	if (!pProcessor)
		return EPerceptionResult::Fail;

	const PerceptionConfig& CONFIG = pProcessor->GetPerceptionConfig();
	const Vector3 TOP(
		_context.intermediate.obstacleHitPos.x,
		_context.intermediate.obstacleLedge + CONFIG.depthLift,
		_context.intermediate.obstacleHitPos.z
	);
	
	Vector3 dir(0.0f);
	PerceptionNodeUtil::LocalizeDirection(_context.owner->GetRoot()->localTransform, GetDirection(), dir);

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
		if (_context.physics->Raycast(param, result, ToMask(Layer::Obstacle)) == false)
			break; // 못 딛는 구간

		if (PerceptionNodeUtil::ToIObstacle(result.GetActor()) != _context.intermediate.pObstacle)
			break;

		depth = CONFIG.depthStep * i;
	}

	_context.intermediate.obstacleDepth = depth;

	return EPerceptionResult::Succeess;
}


/// 최종 관측 지점에서 한번 더 레이를 쏴서 여유 공간이 있는지 확인
EPerceptionResult CheckRoomNode::InvokeTask(TravelContext& _context, PerceptResult& _result)
{
	std::shared_ptr<IPerceptionProcessor> pProcessor = std::dynamic_pointer_cast<IPerceptionProcessor>(_context.owner);
	if (!pProcessor)
		return EPerceptionResult::Fail;

	const Transform& TF = pProcessor->GetTransform();
	const Vector3& PROBE_XZ = _context.intermediate.obstacleHitPos;

	SpherecastParam param;
	PerceptionNodeUtil::LocalizeDirection(TF, GetDirection(), param.m_dir);
	param.m_radius = m_radius;
	param.m_maxDistance = m_distance;
	param.m_startPos = m_startOffset + Vector3(PROBE_XZ.x, _context.intermediate.obstacleLedge, PROBE_XZ.z);

#if MG_DEBUG_LOG
	Vector3 endPos = param.m_startPos + param.m_dir * param.m_maxDistance;
	MiniEngine::Debug::DrawLine(param.m_startPos, endPos, MiniEngine::DebugColor::GREEN, 1.0f);
	MiniEngine::Debug::DrawPoint(endPos, MiniEngine::DebugColor::GREEN, param.m_radius, MiniEngine::Debug::EMarkerShape::Sphere, 1.0f);
#endif // MG_DEBUG_LOG

	RaycastResult result;
	if (_context.physics->SphereCast(param, result, ToMask(MiniEngine::Physics::Layer::Obstacle)))
	{
		_context.intermediate.roomHeight = result.m_pos.y - param.m_startPos.y + m_startOffset.y;
		MG_LOG_INFO("[CheckRoomNode::InvokeTask] Not Enough Room : {}", _context.intermediate.roomHeight);
	}
	
	return EPerceptionResult::Succeess;
}
