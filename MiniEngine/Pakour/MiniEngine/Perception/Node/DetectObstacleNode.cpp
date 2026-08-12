#include "pch.h"
#include "Perception/Node/DetectObstacleNode.h"
#include "Perception/Node/PerceptionNodeUtil.h"
#include "Perception/Interface/IObstacle.h"
#include "Perception/Interface/IPerceptionProcessor.h"
#include "Scene/Actor.h"
#include "Physics/PhysicsWorld.h"

#include "Core/DebugMarkers.h"
#include "Core/Log.h"

using namespace MiniEngine::Physics;
using namespace PerceptionNodeUtil;

void DetectNode::SortResults(RaycastMultipleResult& _result) const
{
	std::sort(_result.m_hitResults.begin(), _result.m_hitResults.end(),
		[this](const HitResult& _a, const HitResult& _b)
		{
			IObstacle* pA = ToIObstacle(_a.GetActor());
			IObstacle* pB = ToIObstacle(_b.GetActor());
			if (!pA || !pB)
				return true;

			if (pA->GetPriority() == pB->GetPriority())
				return _a.m_distance < _b.m_distance;

			return pA->GetPriority() > pB->GetPriority();
		}
	);
}

void DetectNode::FilterResults(
	std::shared_ptr<IPerceptionProcessor>& _pProcessor, 
	TravelContext& _context, 
	MiniEngine::Physics::RaycastMultipleResult& _result) const
{
	void* pOverlappedObstacle = nullptr;
	if (_pProcessor->GetCurObstacleInfo().IsValid())
		pOverlappedObstacle = dynamic_cast<void*>(_pProcessor->GetCurObstacle());

	for (const HitResult& r : _result.m_hitResults)
	{
		if (pOverlappedObstacle != nullptr && r.GetActor() == pOverlappedObstacle)
			continue; // 현재 처리 중인 장애물 -> 다음 후보로

		// 현재 부딪힌 포인트 저장
		FillFromResult(_context, r);
		break;
	}
}

void DetectNode::ApplyOwnerTransform(const Transform& _inOwnerTf, Vector3& _outPos, Vector3& _outDir) const
{
	LocalizePosition(_inOwnerTf, GetStartOffset(), _outPos);
	LocalizeDirection(_inOwnerTf, GetDirection(), _outDir);
}

EPerceptionResult DetectObstacleCapsuleNode::InvokeTask(TravelContext& _context, TravelResult& _result)
{
	const Transform& TF = _context.m_owner->GetRoot()->localTransform;
	std::shared_ptr<IPerceptionProcessor> pProcessor = std::dynamic_pointer_cast<IPerceptionProcessor>(_context.m_owner);

	if (!pProcessor)
		return EPerceptionResult::Fail;

	const PerceptionConfig& CONFIG = pProcessor->GetPerceptionConfig();

	// 지오메트리 생성
	CapsulecastParam param;
	param.m_radius			= GetRadius();
	param.m_halfHeight		= m_capsuleHeight * 0.5f * m_heightMultiplier;
	param.m_maxDistance		= GetDistance();

	// Owner 트랜스폼 기준 적용
	ApplyOwnerTransform(TF, param.m_startPos, param.m_dir);

	Vector3 debugEnd = param.m_startPos + param.m_dir * param.m_maxDistance;
	MiniEngine::Debug::DrawLine(param.m_startPos, debugEnd, MiniEngine::DebugColor::YELLOW, 1.0f);

	Vector3 debugPointLow = debugEnd - Vector3(0.0f, param.m_halfHeight, 0.0f);
	Vector3 debugPointHigh = debugEnd + Vector3(0.0f, param.m_halfHeight, 0.0f);
	MiniEngine::Debug::DrawLine(debugPointLow, debugPointHigh, MiniEngine::DebugColor::YELLOW, 1.0f);
	MiniEngine::Debug::DrawPoint(debugPointLow, MiniEngine::DebugColor::YELLOW, param.m_radius, MiniEngine::Debug::EMarkerShape::Sphere, 1.0f);
	MiniEngine::Debug::DrawPoint(debugPointHigh, MiniEngine::DebugColor::YELLOW, param.m_radius, MiniEngine::Debug::EMarkerShape::Sphere, 1.0f);

	// 1. 특정 방향에 장애물 유무 확인
	RaycastMultipleResult hits;
	if (_context.m_physics->CapsuleCastMultiple(param, hits, ToMask(Layer::Obstacle)) == false)
		return EPerceptionResult::Succeess;

	// 2. 우선순위 + 거리에 따른 정렬
	SortResults(hits);

	// 3. 추가 필터링 : 현재 처리 중인지 장애물 확인
	FilterResults(pProcessor, _context, hits);

	return EPerceptionResult::Succeess;
}

EPerceptionResult DetectObstacleSphereNode::InvokeTask(TravelContext& _context, TravelResult& _result)
{
	const Transform& TF = _context.m_owner->GetRoot()->localTransform;
	std::shared_ptr<IPerceptionProcessor> pProcessor = std::dynamic_pointer_cast<IPerceptionProcessor>(_context.m_owner);

	if (!pProcessor)
		return EPerceptionResult::Fail;

	const PerceptionConfig& CONFIG = pProcessor->GetPerceptionConfig();

	// 지오메트리 생성
	SpherecastParam param;
	param.m_radius = GetRadius();
	param.m_maxDistance = GetDistance();

	// Owner 트랜스폼 기준 적용
	ApplyOwnerTransform(TF, param.m_startPos, param.m_dir);

	// 1. 특정 방향에 장애물 유무 확인
	RaycastMultipleResult hits;
	if (_context.m_physics->SphereCastMultiple(param, hits, ToMask(Layer::Obstacle)) == false)
		return EPerceptionResult::Succeess;

	// 2. 우선순위 + 거리에 따른 정렬
	SortResults(hits);

	// 3. 추가 필터링 : 현재 처리 중인지 장애물 확인
	FilterResults(pProcessor, _context, hits);

	return EPerceptionResult::Succeess;
}

EPerceptionResult DetectLedgeNode::InvokeTask(TravelContext& _context, TravelResult& _result)
{
	SpherecastParam param;
	param.m_radius = GetRadius();
	param.m_maxDistance = GetDistance();
	ApplyOwnerTransform(_context.m_owner->GetRoot()->localTransform, param.m_startPos, param.m_dir);

	RaycastResult result;
	if (_context.m_physics->SphereCast(param, result, ToMask(Layer::ObstacleLedge)))
	{
		FillFromResult(_context, result);
		_context.m_bDetectLedge = true;
	}

	return EPerceptionResult::Succeess;
}

EPerceptionResult DetectLedgeMultipleNode::InvokeTask(TravelContext& _context, TravelResult& _result)
{
	SpherecastParam param;
	param.m_radius = GetRadius();
	param.m_maxDistance = GetDistance();
	ApplyOwnerTransform(_context.m_owner->GetRoot()->localTransform, param.m_startPos, param.m_dir);

	RaycastMultipleResult result;
	bool bIsHit = _context.m_physics->SphereCastMultiple(param, result, ToMask(Layer::ObstacleLedge));

	if (bIsHit == false)
		return EPerceptionResult::Succeess;

	std::sort(result.m_hitResults.begin(), result.m_hitResults.end(),
		[](const HitResult& _a, const HitResult& _b) { return _a.m_pos.y > _b.m_pos.y; } // 접촉 높이가 내림차순 정렬
	);

	RaycastResult firstResult;
	firstResult.FillFromHitResult(result.m_bIsHit, result.m_hitResults.front());
	FillFromResult(_context, firstResult);
	_context.m_bDetectLedge = true;

	return EPerceptionResult::Succeess;
}



EPerceptionResult CheckObstacleSphereNode::InvokeTask(TravelContext& _context, TravelResult& _result)
{
	const Transform& TF = _context.m_owner->GetRoot()->localTransform;
	std::shared_ptr<IPerceptionProcessor> pProcessor = std::dynamic_pointer_cast<IPerceptionProcessor>(_context.m_owner);

	if (!pProcessor)
		return EPerceptionResult::Fail;

	// 지오메트리 생성
	SpherecastParam param;
	param.m_radius = GetRadius();
	param.m_maxDistance = GetDistance();

	// Owner 트랜스폼 기준 적용
	ApplyOwnerTransform(TF, param.m_startPos, param.m_dir);

	RaycastResult result;
	if (_context.m_physics->SphereCast(param, result, ToMask(Layer::Obstacle)) == false)
	{
		_context.m_pFirstObstacle = nullptr;
		return EPerceptionResult::Succeess;
	}

	FillFromResult(_context, result);

	return EPerceptionResult::Succeess;
}


EPerceptionResult DetectFloorNode::InvokeTask(TravelContext& _context, TravelResult& _result)
{
	const Transform& TF = _context.m_owner->GetRoot()->localTransform;
	std::shared_ptr<IPerceptionProcessor> pProcessor = std::dynamic_pointer_cast<IPerceptionProcessor>(_context.m_owner);

	if (!pProcessor)
		return EPerceptionResult::Fail;

	// 지오메트리 생성
	SpherecastParam param;
	param.m_radius = GetRadius();
	param.m_maxDistance = GetDistance();

	// Owner 트랜스폼 기준 적용
	ApplyOwnerTransform(TF, param.m_startPos, param.m_dir);

	RaycastResult result;
	if (_context.m_physics->SphereCast(param, result, Layer::Obstacle | Layer::Ground) == false)
	{
		_context.m_pFirstObstacle = nullptr;
		return EPerceptionResult::Succeess;
	}

	FillFromResult(_context, result);

	return EPerceptionResult::Succeess;
}
