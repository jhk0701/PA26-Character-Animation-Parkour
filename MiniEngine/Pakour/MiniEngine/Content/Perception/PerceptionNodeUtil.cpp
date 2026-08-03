#include "pch.h"
#include "Content/Perception/PerceptionNodeUtil.h"
#include "Scene/IObstacle.h"
#include "Physics/PhysicsWorld.h"

#include "Content/ContentConfig.h"
#include "Content/Character.h"
#include "Content/Obstacle.h"
#include "Core/DebugMarkers.h"

using namespace MiniEngine;
using namespace MiniEngine::Physics;
using namespace Content::Config;

namespace PerceptionNodeUtil 
{
	std::shared_ptr<Character> ToChar(std::shared_ptr<Actor> _actor)
	{
		return std::dynamic_pointer_cast<Character>(_actor);
	}

	Vector3 GetCharacterCenterPosition(TravelContext& _context) 
	{
		std::shared_ptr<Character> pChar = ToChar(_context.m_owner);
		if (!pChar)
			return Vector3(0.0f);

		return pChar->GetRoot()->localTransform.position + Vector3(0.0f, pChar->GetCharacterHalfHeight(), 0.0f);
	}

	IObstacle* ToIObstacle(void* _p) 
	{
		Actor* pActor = reinterpret_cast<Actor*>(_p);

		if (!pActor)
			return nullptr;

		return dynamic_cast<IObstacle*>(pActor);
	}
	bool CheckObstacle(TravelContext& _context, 
		const Vector3& _pos, 
		const Vector3& _dir, 
		const float _dist, 
		const float _hMultiplier, 
		const bool _bExcludeGroundActor)
	{
		std::shared_ptr<Character> pChar = ToChar(_context.m_owner);
		const Transform& TF = pChar->GetRoot()->localTransform;

		CapsulecastParam capParam;
		capParam.m_radius = pChar->GetCapsuleRadius();
		capParam.m_halfHeight = pChar->GetCapsuleHalfHeight() * _hMultiplier;
		capParam.m_startPos = _pos;
		capParam.m_dir = _dir;
		capParam.m_maxDistance = _dist;

		// 결과물은 거리 순으로 정렬해서 보내줌
		RaycastMultipleResult hits;
		if (_context.m_physics->CapsuleCastMultiple(capParam, hits, ToMask(Layer::Obstacle)) == false)
			return false;

		// 현재 처리 중엔 장애물 확인
		void* pOverlappedObstacle = nullptr;
		if (pChar->GetCurObstacleInfo().IsValid())  
			pOverlappedObstacle = reinterpret_cast<void*>(pChar->GetCurObstacleInfo().m_pObstacle);

		// 지금 장애물 위에 있는지 확인
		void* pGroundActor = nullptr;
		if (_bExcludeGroundActor)
		{
			SpherecastParam spParam;
			spParam.m_startPos = TF.position;
			spParam.m_radius = capParam.m_radius;
			spParam.m_dir = Vector3(0.0f, -1.0f, 0.0f);
			spParam.m_maxDistance = 0.1f;

			RaycastResult downCheckResult;
			if (_context.m_physics->SphereCast(spParam, downCheckResult, ToMask(Layer::Obstacle)))
				pGroundActor = downCheckResult.GetActor();
		}

		for (const HitResult& r : hits.m_hitResults)
		{
			if (pGroundActor != nullptr && r.GetActor() == pGroundActor || 
				pOverlappedObstacle != nullptr && r.GetActor() == pOverlappedObstacle)
				continue; // 이미 올라온 장애물, 현재 처리 중인 장애물 -> 다음 후보로

			FillFromResult(_context, r); // 높이는 MeasureObstacleHeight 가 다시 잰다
			return true;
		}

		return false; // 딛고 선 장애물 외에 아무것도 없음 -> 찾지 못한 것
	}

	bool CheckLedge(MiniEngine::TravelContext& _context, const MiniEngine::Vector3& _pos, const MiniEngine::Vector3& _dir, const float _radius, MiniEngine::Physics::RaycastResult& _outResult)
	{
		SpherecastParam sphParam;
		sphParam.m_startPos = _pos;
		sphParam.m_dir = _dir;
		sphParam.m_radius = _radius;
		sphParam.m_maxDistance = ToChar(_context.m_owner)->GetPerceptionConfig().minObstacleDetectDist;

		bool bIsHit = _context.m_physics->SphereCast(sphParam, _outResult, ToMask(Layer::ObstacleLedge));

		if (bIsHit)
			_context.m_ledge = _outResult.m_pos.y;

		return bIsHit;
	}

	uint8_t MeasureObstacleHeight(MiniEngine::TravelContext& _context, const MiniEngine::Vector3& _dir)
	{
		std::shared_ptr<Character> pChar = ToChar(_context.m_owner);
		const PerceptionConfig& CONFIG = pChar->GetPerceptionConfig();
		const float FOOT_Y = pChar->GetRoot()->localTransform.position.y;
		const Vector3& PROBE_XZ = _context.m_firstObstacleHitPos;

		uint8_t band = 0;
		bool bFirstTouched = false;

		for (; band < CONFIG.maxHeightStep; ++band)
		{
			SpherecastParam param;
			param.m_startPos = Vector3(
				PROBE_XZ.x,
				FOOT_Y + CONFIG.heightRadius + band * CONFIG.heightStep,
				PROBE_XZ.z);
			param.m_radius = CONFIG.heightRadius;
			param.m_dir = _dir;
			param.m_maxDistance = CONFIG.heightSearchtDist;

			Vector3 debugEnd = param.m_startPos + param.m_dir * param.m_maxDistance;
			
			MiniEngine::Debug::DrawLine(param.m_startPos, debugEnd, MiniEngine::DebugColor::YELLOW, 1.0f);
			
			RaycastResult result;
			if (_context.m_physics->SphereCast(param, result, ToMask(Layer::Obstacle)) == false)
			{
				MiniEngine::Debug::DrawPoint(param.m_startPos, MiniEngine::DebugColor::RED, param.m_radius, MiniEngine::Debug::EMarkerShape::Sphere, 1.0f);
				
				// 최소 한번 닿았음 근데, hit가 끊어짐 -> 최고점 도달 처리
				if (bFirstTouched) 
					break; // 닿지 않음
			}
			else
			{
				if (!bFirstTouched)
					bFirstTouched = true;

				MiniEngine::Debug::DrawPoint(param.m_startPos, MiniEngine::DebugColor::GREEN, param.m_radius, MiniEngine::Debug::EMarkerShape::Sphere, 1.0f);
			}
		}

		_context.m_ledge = FOOT_Y + band * CONFIG.heightStep;

		if (band > 0)
		{
			const Vector3 LEDGE_ORIGIN(PROBE_XZ.x, _context.m_ledge - CONFIG.heightRadius, PROBE_XZ.z);

			RaycastResult ledgeResult;
			CheckLedge(_context, LEDGE_ORIGIN, _dir, CONFIG.heightRadius, ledgeResult);
		}

		return band;
	}

	void MeasureObstacleDepth(MiniEngine::TravelContext& _context, const MiniEngine::Vector3& _dir)
	{
		std::shared_ptr<Character> pChar = ToChar(_context.m_owner);
		const PerceptionConfig& CONFIG = pChar->GetPerceptionConfig();

		const Vector3 TOP(
			_context.m_firstObstacleHitPos.x,
			_context.m_ledge + CONFIG.depthLift,
			_context.m_firstObstacleHitPos.z
		);

		float depth = 0.0f;
		for (uint8_t i = 1; i <= CONFIG.maxDepthStep; ++i)
		{
			RaycastParam param;
			param.m_origin = TOP + _dir * (CONFIG.depthStep * i);
			param.m_dir = Vector3(0.0f, -1.0f, 0.0f);
			param.m_maxDistance = CONFIG.depthSearchDownDist;

			RaycastResult result;
			if (_context.m_physics->Raycast(param, result, ToMask(Layer::Obstacle)) == false)
				break; // 구멍 -> 여기서부터는 딛을 수 없다

			// 마스크가 Obstacle-only 라 지면은 안 맞지만 뒤에 있는 다른 장애물은 맞는다.
			// 그걸 깊이로 세면 얇은 난간이 Mantle 로 새므로 액터 동일성으로 막는다
			if (ToIObstacle(result.GetActor()) != _context.m_pFirstObstacle)
				break;

			depth = CONFIG.depthStep * i;
		}

		_context.m_depth = depth;
	}
}

