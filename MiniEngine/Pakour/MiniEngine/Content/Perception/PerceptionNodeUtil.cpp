#include "pch.h"
#include "Content/Perception/PerceptionNodeUtil.h"
#include "Scene/IObstacle.h"
#include "Physics/PhysicsWorld.h"

#include "Content/ContentConfig.h"
#include "Content/Character.h"
#include "Content/Data/CharacterPerceptionConfig.h"
#include "Content/Obstacle.h"
#include "Core/DebugMarkers.h"
#include "Core/Log.h"

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

	MiniEngine::Vector3 GetCharacterHeadPosition(MiniEngine::TravelContext& _context)
	{
		std::shared_ptr<Character> pChar = ToChar(_context.m_owner);
		if (!pChar)
			return Vector3(0.0f);
		
		return pChar->GetRoot()->localTransform.position + Vector3(0.0f, pChar->GetCharacterHeight(), 0.0f);
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
		const PerceptionConfig& CONFIG = pChar->GetPerceptionConfig();

		CapsulecastParam capParam;
		capParam.m_radius = pChar->GetCapsuleRadius();
		capParam.m_halfHeight = pChar->GetCapsuleHalfHeight() * _hMultiplier;
		capParam.m_startPos = _pos;
		capParam.m_startPos.y += CONFIG.heightLift;
		capParam.m_dir = _dir;
		capParam.m_maxDistance = _dist;

		/*
		Vector3 debugEnd = capParam.m_startPos + capParam.m_dir * capParam.m_maxDistance;
		MiniEngine::Debug::DrawLine(capParam.m_startPos, debugEnd, MiniEngine::DebugColor::YELLOW, 0.5f);
		*/

		// 결과물은 거리 순으로 정렬해서 보내줌
		RaycastMultipleResult hits;
		if (_context.m_physics->CapsuleCastMultiple(capParam, hits, ToMask(Layer::Obstacle)) == false)
			return false;

		// 현재 처리 중엔 장애물 확인
		void* pOverlappedObstacle = nullptr;
		if (pChar->GetCurObstacleInfo().IsValid())  
			pOverlappedObstacle = dynamic_cast<void*>(pChar->GetCurrentObstacle());

		// 지금 장애물 위에 있는지 확인
		void* pGroundActor = nullptr;
		if (_bExcludeGroundActor)
		{
			SpherecastParam spParam;
			spParam.m_startPos = TF.position;
			spParam.m_startPos.y += CONFIG.heightLift;

			spParam.m_radius = capParam.m_radius;
			spParam.m_dir = Vector3(0.0f, -1.0f, 0.0f);
			spParam.m_maxDistance = 0.1f;

			RaycastResult downCheckResult;
			if (_context.m_physics->SphereCast(spParam, downCheckResult, ToMask(Layer::Obstacle)))
				pGroundActor = downCheckResult.GetActor();
		}

		for (const HitResult& r : hits.m_hitResults)
		{
			if ((pGroundActor != nullptr && r.GetActor() == pGroundActor) ||
				(pOverlappedObstacle != nullptr && r.GetActor() == pOverlappedObstacle))
			{
				MG_LOG_INFO("[CheckObstacleSphere] is overlapped -> skip, {}", hits.m_hitResults.size());
				continue; // 이미 올라온 장애물, 현재 처리 중인 장애물 -> 다음 후보로
			}

			FillFromResult(_context, r); // 높이는 MeasureObstacleHeight 가 다시 잰다
			return true;
		}

		return false; // 딛고 선 장애물 외에 아무것도 없음 -> 찾지 못한 것
	}

	bool CheckObstacleSphere(
		MiniEngine::TravelContext& _context, 
		const MiniEngine::Vector3& _pos, 
		const MiniEngine::Vector3& _dir, 
		const float _dist, 
		const float _radius, 
		const bool _bExcludeGroundActor)
	{
		std::shared_ptr<Character> pChar = ToChar(_context.m_owner);
		const Transform& TF = pChar->GetRoot()->localTransform;
		const PerceptionConfig& CONFIG = pChar->GetPerceptionConfig();

		SpherecastParam sphParam;
		sphParam.m_radius = _radius;
		sphParam.m_startPos = _pos;
		sphParam.m_dir = _dir;
		sphParam.m_maxDistance = _dist;

		/*Vector3 debugEnd = sphParam.m_startPos + sphParam.m_dir * sphParam.m_maxDistance;
		MiniEngine::Debug::DrawLine(sphParam.m_startPos, debugEnd, MiniEngine::DebugColor::YELLOW, 0.5f);
		MiniEngine::Debug::DrawPoint(debugEnd, MiniEngine::DebugColor::YELLOW, _radius, MiniEngine::Debug::EMarkerShape::Sphere, 0.5f);*/

		// 결과물은 거리 순으로 정렬해서 보내줌
		RaycastMultipleResult hits;
		if (_context.m_physics->SphereCastMultiple(sphParam, hits, ToMask(Layer::Obstacle)) == false)
			return false;

		// 현재 처리 중엔 장애물 확인
		void* pOverlappedObstacle = nullptr;
		if (pChar->GetCurObstacleInfo().IsValid())
			pOverlappedObstacle = dynamic_cast<void*>(pChar->GetCurrentObstacle());

		// 지금 장애물 위에 있는지 확인
		void* pGroundActor = nullptr;
		if (_bExcludeGroundActor)
		{
			SpherecastParam spParam;
			spParam.m_startPos = TF.position;
			spParam.m_startPos.y += CONFIG.heightLift;

			spParam.m_radius = pChar->GetCapsuleRadius();
			spParam.m_dir = Vector3(0.0f, -1.0f, 0.0f);
			spParam.m_maxDistance = 0.1f;

			RaycastResult downCheckResult;
			if (_context.m_physics->SphereCast(spParam, downCheckResult, ToMask(Layer::Obstacle)))
				pGroundActor = downCheckResult.GetActor();
		}

		MG_LOG_INFO("[CheckObstacleSphere] hit result {}", hits.m_hitResults.size());
		for (const HitResult& r : hits.m_hitResults)
		{
			if ((pGroundActor != nullptr && r.GetActor() == pGroundActor) ||
				(pOverlappedObstacle != nullptr && r.GetActor() == pOverlappedObstacle))
			{
				MG_LOG_INFO("[CheckObstacleSphere] is overlapped -> skip");
				continue; // 이미 올라온 장애물, 현재 처리 중인 장애물 -> 다음 후보로
			}

			FillFromResult(_context, r); // 높이는 MeasureObstacleHeight 가 다시 잰다
			return true;
		}

		return false; // 딛고 선 장애물 외에 아무것도 없음 -> 찾지 못한 것
	}

	bool CheckLedgeSingle(
		MiniEngine::TravelContext& _context, 
		const MiniEngine::Vector3& _pos, 
		const MiniEngine::Vector3& _dir, 
		const float _radius,
		const float _dist,
		MiniEngine::Physics::RaycastResult& _outResult
	)
	{
		SpherecastParam sphParam;
		sphParam.m_startPos = _pos;
		sphParam.m_dir = _dir;
		sphParam.m_radius = _radius;
		sphParam.m_maxDistance = _dist;

		MiniEngine::Debug::DrawPoint(sphParam.m_startPos, MiniEngine::DebugColor::RED, sphParam.m_radius, MiniEngine::Debug::EMarkerShape::Sphere, 1.0f);

		bool bIsHit = _context.m_physics->SphereCast(sphParam, _outResult, ToMask(Layer::ObstacleLedge));
		if (bIsHit)
		{
			_context.m_ledge = _outResult.m_pos.y;
			_context.m_bDetectLedge = true;
		}

		return bIsHit;
	}

	bool CheckLedgeMultiple(
		TravelContext& _context,
		const Vector3& _pos, 
		const Vector3& _dir, 
		const float _radius, 
		const float _dist, 
		RaycastResult& _outResult
	)
	{
		SpherecastParam sphParam;
		sphParam.m_startPos = _pos;
		sphParam.m_dir = _dir;
		sphParam.m_radius = _radius;
		sphParam.m_maxDistance = _dist;

		RaycastMultipleResult result;
		bool bIsHit = _context.m_physics->SphereCastMultiple(sphParam, result, ToMask(Layer::ObstacleLedge));

		if (bIsHit == false)
			return bIsHit;

		std::sort(result.m_hitResults.begin(), result.m_hitResults.end(), 
			[](const HitResult& _a, const HitResult& _b) { return _a.m_pos.y > _b.m_pos.y; } // 접촉 높이가 내림차순 정렬
		);
		
		_outResult.FillFromHitResult(result.m_bIsHit, result.m_hitResults.front());
		_context.m_ledge = _outResult.m_pos.y;
		_context.m_bDetectLedge = true;

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
				FOOT_Y + CONFIG.heightLift + CONFIG.heightRadius + band * CONFIG.heightStep,
				PROBE_XZ.z);
			param.m_radius = CONFIG.heightRadius;
			param.m_dir = _dir;
			param.m_maxDistance = CONFIG.heightSearchtDist;

			/*
			* Vector3 debugEnd = param.m_startPos + param.m_dir * param.m_maxDistance;
			MiniEngine::Debug::DrawLine(param.m_startPos, debugEnd, MiniEngine::DebugColor::YELLOW, 1.0f);
			MG_LOG_INFO("[PerceptionNodeUtil::MeasureObstacleHeight] Start Pos : ({:.2f}, {:.2f}, {:.2f})", param.m_startPos.x, param.m_startPos.y, param.m_startPos.z);
			*/
			
			RaycastResult result;
			if (_context.m_physics->SphereCast(param, result, ToMask(Layer::Obstacle)) == false)
			{
				// MiniEngine::Debug::DrawPoint(param.m_startPos, MiniEngine::DebugColor::RED, param.m_radius, MiniEngine::Debug::EMarkerShape::Sphere, 1.0f);
				
				// 최소 한번 닿았음 근데, hit가 끊어짐 -> 최고점 도달 처리
				if (bFirstTouched) 
					break; // 닿지 않음
			}
			else
			{
				// MiniEngine::Debug::DrawPoint(param.m_startPos, MiniEngine::DebugColor::GREEN, param.m_radius, MiniEngine::Debug::EMarkerShape::Sphere, 1.0f);

				if (!bFirstTouched)
					bFirstTouched = true;
			}
		}

		if (bFirstTouched)
			_context.m_ledge = FOOT_Y + band * CONFIG.heightStep;
		else
			_context.m_ledge = FOOT_Y;

		if (band > 0)
		{
			const Vector3 LEDGE_ORIGIN(PROBE_XZ.x, _context.m_ledge - CONFIG.heightRadius, PROBE_XZ.z);
			
			RaycastResult ledgeResult;
			CheckLedgeSingle(_context, LEDGE_ORIGIN, _dir, CONFIG.ledgeDetectRadius, CONFIG.minObstacleDetectDist,ledgeResult);
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

			// MiniEngine::Debug::DrawPoint(param.m_origin, MiniEngine::DebugColor::RED, 0.25f, MiniEngine::Debug::EMarkerShape::Sphere, 1.0f);

			RaycastResult result;
			if (_context.m_physics->Raycast(param, result, ToMask(Layer::Obstacle)) == false)
				break; // 구멍 -> 여기서부터는 딛을 수 없다

			if (ToIObstacle(result.GetActor()) != _context.m_pFirstObstacle)
				break;

			depth = CONFIG.depthStep * i;
		}

		_context.m_depth = depth;
	}
}

