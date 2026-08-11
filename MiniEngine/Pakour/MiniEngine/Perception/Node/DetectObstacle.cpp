#include "pch.h"
#include "Perception/Node/DetectObstacle.h"
#include "Perception/Node/PerceptionNodeUtil.h"
#include "Perception/Interface/IObstacle.h"
#include "Perception/Interface/IPerceptionProcessor.h"
#include "Scene/Actor.h"
#include "Physics/PhysicsWorld.h"

#include "Core/DebugMarkers.h"
#include "Core/Log.h"

using namespace MiniEngine::Physics;
using namespace PerceptionNodeUtil;

void DetectObstacle::SortResults(RaycastMultipleResult& _result) const
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

void DetectObstacle::FilterResults(
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

void DetectObstacle::ApplyOwnerTransform(const Transform& _inOwnerTf, Vector3& _outPos, Vector3& _outDir) const
{
	const Vector3& OFFSET = GetStartOffset();
	Vector3 startPos = _inOwnerTf.position;
	startPos += OFFSET.x * _inOwnerTf.Right();
	startPos += OFFSET.y * _inOwnerTf.Up();
	startPos += OFFSET.z * _inOwnerTf.Forward();

	_outPos = startPos;

	const Vector3& DIR = GetDirection();
	Vector3 dir(0.0f);
	dir += DIR.x * _inOwnerTf.Right();
	dir += DIR.y * _inOwnerTf.Up();
	dir += DIR.z * _inOwnerTf.Forward();
	dir.Normalize();

	_outDir = dir;
}

EPerceptionResult DetectObstacleCapsule::InvokeTask(TravelContext& _context, TravelResult& _result)
{
	const Transform& TF = _context.m_owner->GetRoot()->localTransform;
	std::shared_ptr<IPerceptionProcessor> pProcessor = std::dynamic_pointer_cast<IPerceptionProcessor>(_context.m_owner);

	if (!pProcessor)
		return EPerceptionResult::Fail;

	const PerceptionConfig& CONFIG = pProcessor->GetPerceptionConfig();

	// 지오메트리 생성
	CapsulecastParam param;
	param.m_radius			= m_capsuleRadius;
	param.m_halfHeight		= m_capsuleHeight * GetHeightMultiplier();
	param.m_maxDistance		= GetDistance();

	// Owner 트랜스폼 기준 적용
	ApplyOwnerTransform(TF, param.m_startPos, param.m_dir);

	/*
	Vector3 debugEnd = capParam.m_startPos + capParam.m_dir * capParam.m_maxDistance;
	MiniEngine::Debug::DrawLine(capParam.m_startPos, debugEnd, MiniEngine::DebugColor::YELLOW, 0.5f);
	*/

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

EPerceptionResult DetectObstacleSphere::InvokeTask(TravelContext& _context, TravelResult& _result)
{
	const Transform& TF = _context.m_owner->GetRoot()->localTransform;
	std::shared_ptr<IPerceptionProcessor> pProcessor = std::dynamic_pointer_cast<IPerceptionProcessor>(_context.m_owner);

	if (!pProcessor)
		return EPerceptionResult::Fail;

	const PerceptionConfig& CONFIG = pProcessor->GetPerceptionConfig();

	// 지오메트리 생성
	SpherecastParam param;
	param.m_radius = m_radius;
	param.m_maxDistance = GetDistance();

	// Owner 트랜스폼 기준 적용
	ApplyOwnerTransform(TF, param.m_startPos, param.m_dir);

	/*
		Vector3 debugEnd = sphParam.m_startPos + sphParam.m_dir * sphParam.m_maxDistance;
		MiniEngine::Debug::DrawLine(sphParam.m_startPos, debugEnd, MiniEngine::DebugColor::YELLOW, 0.5f);
		MiniEngine::Debug::DrawPoint(debugEnd, MiniEngine::DebugColor::YELLOW, _radius, MiniEngine::Debug::EMarkerShape::Sphere, 0.5f);
	*/

	RaycastMultipleResult hits;
	if (_context.m_physics->SphereCastMultiple(param, hits, ToMask(Layer::Obstacle)) == false)
		return EPerceptionResult::Succeess;

	// 2. 우선순위 + 거리에 따른 정렬
	SortResults(hits);

	// 3. 추가 필터링 : 현재 처리 중인지 장애물 확인
	FilterResults(pProcessor, _context, hits);

	return EPerceptionResult::Succeess;
}