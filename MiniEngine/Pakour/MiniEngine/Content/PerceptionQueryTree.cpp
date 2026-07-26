#include "pch.h"
#include "Content/PerceptionQueryTree.h"
#include "Content/ContentConfig.h"
#include "Content/Character.h"
#include "Scene/PerceptionComponent.h"
#include "Physics/PhysicsWorld.h"
#include "Core/Log.h"

using namespace MiniEngine;
using namespace MiniEngine::Physics;
using namespace Content::Config;


namespace 
{
	// 헬퍼 메서드 모음
	// Config
	constexpr float MIN_OBSTACLE_DETECT_DIST = 1.0f;
	constexpr float MAX_OBSTACLE_DETECT_DIST = 2.5f;
	constexpr float CAPSULE_CONTACT_OFFSET = 0.2f;
	constexpr float MIN_MANTLE_DEPTH_THRESHOLD = 0.75f; // 해당 깊이에서 레이를 쏘았는데 닿았다 -> mantle

	std::shared_ptr<Character> ToChar(std::shared_ptr<Actor> _actor) 
	{
		return std::dynamic_pointer_cast<Character>(_actor);
	}

	float GetCharHeight(TravelContext& _context)
	{
		std::shared_ptr<Character> pChar = ToChar(_context.m_owner);

		if (!pChar)
			return 1.0f;
		
		return pChar->GetCharacterHalfHeight() * 2.0f;
	}

	Vector3 GetCharacterCenterPosition(TravelContext& _context) 
	{
		std::shared_ptr<Character> pChar = ToChar(_context.m_owner);

		if (!pChar)
			return Vector3(0.0f);

		return pChar->GetRoot()->localTransform.position + Vector3(0.0f, pChar->GetCharacterHalfHeight(), 0.0f);
	}

	// 심플하게 결과값 상관없이 히트하는지만 보려는 때 사용
	bool SphereCast(TravelContext& _context, const Vector3& _pos, const Vector3& _dir, const float _dist, const uint32_t _layer)
	{
		std::shared_ptr<Character> pChar = ToChar(_context.m_owner);

		SpherecastParam param;
		param.m_startPos = _pos;
		param.m_dir = _dir;
		param.m_radius = pChar->GetCapsuleRadius();
		param.m_maxDistance = _dist;

		RaycastResult result;
		return _context.m_physics->SphereCast(param, result, _layer);
	}
	bool CapsuleCast(TravelContext& _context, const Vector3& _pos, const Vector3& _dir, const float _dist, const uint32_t _layer)
	{
		std::shared_ptr<Character> pChar = ToChar(_context.m_owner);

		CapsulecastParam param;
		param.m_startPos = _pos;
		param.m_dir = _dir;
		param.m_radius = pChar->GetCapsuleRadius();
		param.m_halfHeight = pChar->GetCapsuleHalfHeight();
		param.m_maxDistance = _dist;

		RaycastResult result;
		return _context.m_physics->CapsuleCast(param, result, _layer);
	}

	// 캐릭터 기준으로 현재 위치에서 특정 방향에 장애물이 있는지 체크
	bool CheckObstacle(TravelContext& _context, const Vector3& _pos, const Vector3& _dir, const float _dist, const float _hMultiplier = 2.0f)
	{
		std::shared_ptr<Character> pChar = ToChar(_context.m_owner);
		const Transform& TF = pChar->GetRoot()->localTransform;

		CapsulecastParam capParam;
		capParam.m_radius = pChar->GetCapsuleRadius();
		capParam.m_halfHeight = pChar->GetCapsuleHalfHeight() * _hMultiplier; //- CAPSULE_CONTACT_OFFSET;
		capParam.m_startPos = _pos; // GetCharacterCenterPosition(_context); // 
		capParam.m_dir = _dir;
		capParam.m_maxDistance = _dist;

		// MG_LOG_INFO("[QueryTree] : Check Obstacle capsule half height : {}", capParam.m_halfHeight);
		// MG_LOG_INFO("[QueryTree] : Check Obstacle : ({}, {}, {})", capParam.m_startPos.x, capParam.m_startPos.y, capParam.m_startPos.z);

		// 결과물은 거리 순으로 정렬해서 보내줌
		RaycastMultipleResult hits;
		bool bIsHit = _context.m_physics->CapsuleCastMultiple(capParam, hits, ToMask(Layer::Obstacle));

		if (bIsHit == false)
			return false;

		// 경사로와 같이, 이미 올라온 장애물이 판정된 경우
		SpherecastParam spParam;
		spParam.m_startPos = TF.position;
		spParam.m_radius = capParam.m_radius;
		spParam.m_dir = Vector3(0.0f, -1.0f, 0.0f);
		spParam.m_maxDistance = 0.1f;

		RaycastResult downCheckResult;
		if (_context.m_physics->SphereCast(spParam, downCheckResult, ToMask(Layer::Obstacle)))
		{
			// 바로 밑에 장애물이 있음
			bool m_bIsEmpty = true;
			for (size_t i = 0; i < hits.m_hitResults.size(); ++i)
			{
				if (hits.m_hitResults[i].GetActor() == downCheckResult.GetActor())
					continue; // 바닥에 접촉한 장애물이 파악한 장애물과 같음

				// 아예 다른 별개의 장애물 식별
				m_bIsEmpty = false;
				const HitResult& r = hits.m_hitResults[i];

				_context.m_pFirstObstacle = reinterpret_cast<Actor*>(r.GetActor());
				_context.m_raycastPos = r.m_pos;
				_context.m_firstObstacleHitPos = r.m_pos;
				_context.m_ledge = r.m_pos.y;
				_context.m_distance = r.m_distance;
				_context.m_units = 1;

				break; // 순회 종료
			}

			if (m_bIsEmpty) 
				return false; // 만약 겹친 장애물 외에 없는 경우 -> 장애물을 찾지 못한 것
		}
		else 
		{
			// 바닥이 있지도 않은 상태에서 장애물을 확인
			const HitResult& r = hits.m_hitResults.front();

			_context.m_pFirstObstacle = reinterpret_cast<Actor*>(r.GetActor());
			_context.m_firstObstacleHitPos = r.m_pos;
			_context.m_firstObstacleHitNrm = r.m_nrm;
			_context.m_distance = r.m_distance;
			_context.m_ledge = r.m_pos.y;
			_context.m_units = 1; // 1 단위 확정
			_context.m_raycastPos = r.m_pos;
		}

		return bIsHit;
	}

	bool CheckVaultable(TravelContext& _context, const Vector3& _initPos, const Vector3& _dir, const float _unit, uint8_t _maxCnt = 2)
	{
		const Vector3 UP = Vector3(0.0f, _unit, 0.0f);
		Vector3 rayPosition = _initPos; 

		for (uint8_t i = 0; i < _maxCnt; ++i)
		{
			rayPosition += UP;
			
			bool bIsHit = CapsuleCast(_context, rayPosition, _dir, MIN_OBSTACLE_DETECT_DIST, ToMask(Layer::Obstacle));
			if (bIsHit)
			{
				MG_LOG_INFO("[QueryTree] Obstacle hit on : ({}, {}, {}), unit : {}", rayPosition.x, rayPosition.y, rayPosition.z, _context.m_units);
				// 이 높이에선 아직 닿음
				_context.m_units++; // 단위 상승
				_context.m_ledge = rayPosition.y; // 대략적인 위치만 기입
			}
			else
			{
				MG_LOG_INFO("[QueryTree] Obstacle valutable : ({}, {}, {}), unit : {}", rayPosition.x, rayPosition.y, rayPosition.z, _context.m_units);
				return true; // 이 높이에선 닿지 않음 -> 넘어갈 수 있음
			}
		}

		return false; // 매달려야 함
	}

	bool CheckLedge(TravelContext& _context, const Vector3& _pos, const Vector3& _dir, const float _radius, RaycastResult& _outResult)
	{
		std::shared_ptr<Character> pChar = ToChar(_context.m_owner);
		
		SpherecastParam sphParam;
		sphParam.m_startPos = _pos;
		sphParam.m_dir = _dir; //
		sphParam.m_radius = _radius;
		sphParam.m_maxDistance = MIN_OBSTACLE_DETECT_DIST;

		bool bIsHit = _context.m_physics->SphereCast(sphParam, _outResult, ToMask(Layer::ObstacleLedge));
		// MG_LOG_INFO("[QueryTree] LedgeFind Cast Origin : ({}, {}, {})", sphParam.m_startPos.x, sphParam.m_startPos.y, sphParam.m_startPos.z);

		if (bIsHit)
		{
			_context.m_ledge = _outResult.m_pos.y;
			/// MG_LOG_INFO("[QueryTree] Ledge Found : ({}, {}, {})", _outResult.m_pos.x, _outResult.m_pos.y, _outResult.m_pos.z);
		}

		return bIsHit;
	}

	bool CheckLandable(TravelContext& _context, const Vector3& _pos, uint32_t _layerMask, float _dist)
	{
		// climbing 테스트를 완료하고 호출될 것
		// 아래방향을 향해 레이캐스트
		RaycastParam rayParam;
		rayParam.m_origin = _pos;
		rayParam.m_dir = Vector3(0.0f, -1.0f, 0.0f);
		rayParam.m_maxDistance = _dist;

		RaycastResult result;
		return _context.m_physics->Raycast(rayParam, result, _layerMask);
	}

	// 양쪽 사이드 확인
	bool CheckSide(TravelContext& _context, RaycastResult& _outResult, bool _bIsRight, uint32_t _layerMask, float _dist)
	{
		std::shared_ptr<Character> pChar = ToChar(_context.m_owner);
		const Transform& TF = pChar->GetRoot()->localTransform;

		CapsulecastParam param;
		param.m_startPos = GetCharacterCenterPosition(_context);
		param.m_dir = _bIsRight ? TF.Right() : -TF.Right();
		param.m_maxDistance = _dist;
		param.m_radius = pChar->GetCapsuleRadius();
		param.m_halfHeight = pChar->GetCapsuleHalfHeight();
		
		// MG_LOG_INFO("[QueryTree] Check Side :: is right? {}, ({},{},{})", _bIsRight ? "R" : "L", param.m_dir.x, param.m_dir.y, param.m_dir.z);

		return _context.m_physics->CapsuleCast(param, _outResult, _layerMask);
	}
}

// 콘텐츠에서 사용할 지형 인식 로직
// TODO: 데이터 객체로 정리할 것
std::shared_ptr<QueryNodeBase> PerceptionQueryTree::ConstructTree()
{
	// Condition
	std::shared_ptr<SelectorNode> pRootQuery = std::make_shared<SelectorNode>();

	// Landing
	std::shared_ptr<ConditionNode> pStateLanding = std::make_shared<ConditionNode>(); // 평지상태 : 장애물 찾기
	std::shared_ptr<SelectorNode> pCheckObstacleTag = std::make_shared<SelectorNode>(); // 장애물 태그 확인
	
	// Obstacle Default
	std::shared_ptr<ConditionNode> pCheckHeight = std::make_shared<ConditionNode>(); // 장애물 높이 확인
	std::shared_ptr<ConditionNode> pObstableIsLandable = std::make_shared<ConditionNode>(); // 장애물을 너머가 평지인지 확인

	// Obstacle Beam
	std::shared_ptr<ConditionNode> pCompareHeight = std::make_shared<ConditionNode>(); // 캐릭터와 장애물의 y 위치 비교

	// InAir
	std::shared_ptr<ConditionNode> pStateInAir = std::make_shared<ConditionNode>();

	// Hanging
	std::shared_ptr<SelectorNode> pStateHanging = std::make_shared<SelectorNode>();
	std::shared_ptr<ConditionNode> pOnHangingUp = std::make_shared<ConditionNode>();
	std::shared_ptr<ConditionNode> pOnHangingUpDetourableObs = std::make_shared<ConditionNode>(); // 천장 우회 가능한지 확인
	std::shared_ptr<ConditionNode> pOnHangingClimbable = std::make_shared<ConditionNode>();

	std::shared_ptr<ConditionNode> pOnHangingDown = std::make_shared<ConditionNode>();
	std::shared_ptr<SelectorNode> pOnHangingCheckObstacleTag = std::make_shared<SelectorNode>(); // 장애물 태그 확인

	std::shared_ptr<ConditionNode> pOnHangingLeft = std::make_shared<ConditionNode>();
	std::shared_ptr<ConditionNode> pOnHangingRight = std::make_shared<ConditionNode>();
	std::shared_ptr<ConditionNode> pOnSideDetected = std::make_shared<ConditionNode>();	// 사이드 레이캐스트 결과 장애물이 감지됨
	std::shared_ptr<ConditionNode> pOnSideEmpty = std::make_shared<ConditionNode>();	// 사이드 레이캐스트 결과, 장애물이 감지되지 않음 // 전방을 향해 레이캐스트	true : empty, false : outer rotate

	// Beam
	std::shared_ptr<ConditionNode> pStateBeam = std::make_shared<ConditionNode>();

	// Leaf
	std::shared_ptr<LeafNode> pEmpty = std::make_shared<LeafNode>(); // 빈 결과 리턴, 탐색 계속 신호
	std::shared_ptr<LeafNode> pReturn = std::make_shared<LeafNode>(); // 결과 리턴

	pEmpty->SetTask(
		[](TravelContext& _context)
		{
			MG_LOG_INFO("[QueryTree] Is Empty");
			TravelResult result;
			return result;
		}
	);

	pReturn->SetTask(
		[](TravelContext& _context)
		{
			TravelResult result;
			result.m_bIsEmpty = false;
			result.m_actTag = _context.m_predictedActTag;
			result.m_units = _context.m_units;
			result.m_pFirstObstacle = _context.m_pFirstObstacle;
			result.m_firstObstacleHitPos = _context.m_firstObstacleHitPos;
			result.m_firstObstacleHitNrm = _context.m_firstObstacleHitNrm;
			result.m_distanceObstacle = _context.m_distance;
			result.m_obstacleLedge = _context.m_ledge;
			return result;
		}
	);
	
	// 루트 확인 : 평지에 있는 상황인지
	pRootQuery->SetCondition(
		[](TravelContext& _ctx) 
		{ 
			std::shared_ptr<Character> pChar = ToChar(_ctx.m_owner);
			if (pChar == nullptr)
				return (uint8_t)0;

			return (uint8_t)pChar->GetState();
		},
		{
			// 배치 순서는 Character EState 순서대로
			pStateLanding,
			pStateInAir,
			pStateHanging,
			pStateBeam,
			pStateBeam
		}
	);
		// 평지에 있는데, 장애물을 발견했는지
		pStateLanding->SetCondition(
			[](TravelContext& _ctx) 
			{ 
				// 정면으로 먼저 확인
				std::shared_ptr<Character> pChar = ToChar(_ctx.m_owner);
				const Transform& TF = pChar->GetRoot()->localTransform;
				const Vector3 POS = GetCharacterCenterPosition(_ctx);

				return CheckObstacle(_ctx, POS, TF.Forward(), MAX_OBSTACLE_DETECT_DIST); 
			},
			pCheckObstacleTag,	// 찾은 경우 태그 확인
			pEmpty			// 찾지 못한 경우 empty return 
		);

			pCheckObstacleTag->SetCondition(
				[](TravelContext& _ctx) 
				{
					uint8_t detailTag = 0;
					_ctx.m_pFirstObstacle->GetTag().GetTagAt(TAG_ENV_DETAIL, detailTag);

					return detailTag;
				}, 
				{
					// ETagEnvDetail 순서
					pCheckHeight,	// Default // 일반 장애물 높이 확인
					pCompareHeight	// Beam
				}
			);

			pCheckHeight->SetCondition(
				[](TravelContext& _ctx) 
				{
					// 장애물의 높이 확인
					const float CHAR_H = GetCharHeight(_ctx);
					const Vector3& FWD = ToChar(_ctx.m_owner)->GetRoot()->localTransform.Forward();

					bool bVaultable = CheckVaultable(_ctx, _ctx.m_raycastPos, FWD, CHAR_H);
					if (bVaultable == false)
					{
						_ctx.m_predictedActTag = (uint8_t)ETagAct::Wall;
						return false;
					}

					// ledge 체크
					const float RADIUS = CHAR_H * 0.25f;

					Vector3 rayPosition = _ctx.m_raycastPos;
					rayPosition.y += (_ctx.m_units - 1) * CHAR_H;

					// 상반신 크기 확인
					RaycastResult result;
					if (CheckLedge(_ctx, rayPosition + Vector3(0.0f, RADIUS, 0.0f), FWD, RADIUS, result))
						return bVaultable;

					// 하반신 크기 확인
					if (CheckLedge(_ctx, rayPosition - Vector3(0.0f, RADIUS, 0.0f), FWD, RADIUS, result))
						return bVaultable;

					// 배치되지 않은 구조물
					_ctx.m_ledge = rayPosition.y;
					return bVaultable;
				},
				pObstableIsLandable,
				pReturn
			);

				pObstableIsLandable->SetCondition(
					[](TravelContext& _ctx)
					{
						std::shared_ptr<Character> pChar = ToChar(_ctx.m_owner);
						const float CHAR_H = GetCharHeight(_ctx);

						Vector3 rayPosition = _ctx.m_raycastPos; // 최초 장애물 접촉점
						rayPosition.y += _ctx.m_units * CHAR_H;
						rayPosition += pChar->GetRoot()->localTransform.Forward() * MIN_MANTLE_DEPTH_THRESHOLD;
						// MG_LOG_INFO("[QueryTree] Check Landable ({}, {}, {})", rayPosition.x, rayPosition.y, rayPosition.z);

						bool bIsLandable = CheckLandable(_ctx, rayPosition, ToMask(Layer::Obstacle), CHAR_H);

						// 원래는 여기서 Vault / Hurdle 중에 갈려야함
						// 모션이 제한적이라 우선 Mantle, Vault를 사용
						_ctx.m_predictedActTag = (uint8_t)(bIsLandable ? ETagAct::Mantle : ETagAct::Vault) + _ctx.m_units;

						if (bIsLandable)
							MG_LOG_INFO("[QueryTree] Mantle + {}", _ctx.m_units);
						else
							MG_LOG_INFO("[QueryTree] Vault + {}", _ctx.m_units);

						return bIsLandable;
					},
					pReturn,
					pReturn
				);

			pCompareHeight->SetCondition(
				[](TravelContext& _ctx) 
				{
					// 장애물 y 위치 비교
					std::shared_ptr<Character> pChar = ToChar(_ctx.m_owner);
					Actor* pObs = _ctx.m_pFirstObstacle;

					const Vector3& CHAR_POS = GetCharacterCenterPosition(_ctx);
					const Vector3& OBS_POS = pObs->GetRoot()->localTransform.position;

					bool bStepable = OBS_POS.y <= CHAR_POS.y;
					_ctx.m_predictedActTag = (uint8_t)(bStepable ? ETagAct::BeamStand : ETagAct::BeamHanging);
					_ctx.m_ledge = OBS_POS.y;

					MG_LOG_INFO("[QueryTree] Beam Obstacle is found : {}", bStepable ? "will step" : "will hang");
					return bStepable;
				},
				pReturn,
				pReturn
			);

	// Hanging State 처리
	pStateHanging->SetCondition(
		[](TravelContext& _ctx) 
			{
				std::shared_ptr<Character> pChar = ToChar(_ctx.m_owner);
				const Vector2 INPUT_DIR = pChar->GetInputDir();

				if (INPUT_DIR.y > 0)
					return 0;
				else if (INPUT_DIR.y < 0)
					return 1;
				else if (INPUT_DIR.x > 0)
					return 2;
				else if (INPUT_DIR.x < 0)
					return 3;

				return 4;
			},
		{
			pOnHangingUp,
			pOnHangingDown,
			pOnHangingRight,
			pOnHangingLeft,
			pEmpty
		}
	);

		pOnHangingUp->SetCondition(
			[](TravelContext& _ctx) 
			{
				// 윗방향 확인
				// 천장 따위로 막혀있는지
				std::shared_ptr<Character> pChar = ToChar(_ctx.m_owner);
				const Transform& TF = pChar->GetRoot()->localTransform;
				const Vector3 POS = GetCharacterCenterPosition(_ctx);

				return CheckObstacle(_ctx, POS, TF.Up(), MAX_OBSTACLE_DETECT_DIST);
			},
			pOnHangingUpDetourableObs,	// 천장을 우회해서 타고 올라갈 수 있는지 확인
			pOnHangingClimbable			// 천장 없음 climbable 확인
		);
			
			pOnHangingUpDetourableObs->SetCondition(
				[](TravelContext& _ctx) 
				{
					MG_LOG_INFO("[QueryTree] Ceiling is detected. Check detour.");

					// 천장 우회 가능한지
					// 뒤로 1단위 물러나서 다시 위로 레이캐스트
					std::shared_ptr<Character> pChar = ToChar(_ctx.m_owner);
					const Transform& TF = pChar->GetRoot()->localTransform;

					SpherecastParam param;
					param.m_dir = Vector3(0.0f, 1.0f, 0.0f);
					param.m_startPos = _ctx.m_raycastPos - TF.Forward();
					param.m_radius = pChar->GetCapsuleRadius();
					param.m_maxDistance = MIN_OBSTACLE_DETECT_DIST;

					RaycastResult result;
					bool bIsHit = _ctx.m_physics->SphereCast(param, result, ToMask(Layer::Obstacle));
					if (bIsHit == false)
					{
						// MG_LOG_INFO("[QueryTree] Can detour");
						_ctx.m_predictedActTag = (uint8_t)ETagAct::Wall_HangToMantle;

						// Ledge 찾기
						if (bIsHit = CheckLedge(_ctx, param.m_startPos, param.m_dir, param.m_radius * 2.0f, result))
							_ctx.m_ledge = result.m_pos.y;
					} 

					return true;
				},
				pReturn,	// 우회접근 가능
				pEmpty		// 불가능할 시 아무 행동 x
			);

			pOnHangingClimbable->SetCondition(
				[](TravelContext& _ctx) 
				{
					std::shared_ptr<Character> pChar = ToChar(_ctx.m_owner);
					const Vector3 POS = GetCharacterCenterPosition(_ctx) + Vector3(0.0f, 1.0f, 0.0f);
					const Vector3 DIR = pChar->GetRoot()->localTransform.Forward();

					// MG_LOG_INFO("[QueryTree] Check Ledge to climbing :: Dir ({}, {}, {})", DIR.x, DIR.y, DIR.z);
					RaycastResult result;
					bool bIsHit = CheckLedge(_ctx, POS, DIR, 1.0f, result);
					if (bIsHit)
					{
						_ctx.m_pFirstObstacle = reinterpret_cast<Actor*>(result.GetActor());
						_ctx.m_firstObstacleHitPos = result.m_pos;
						_ctx.m_distance = result.m_distance;
						_ctx.m_ledge = result.m_pos.y;

						// MG_LOG_INFO("[QueryTree] Wall Ledge Climb");
						_ctx.m_predictedActTag = (uint8_t)ETagAct::Wall_HangToMantle;
					}

					return bIsHit;
				},
				pReturn,
				pEmpty
			);

		pOnHangingDown->SetCondition(
			[](TravelContext& _ctx)
			{
				std::shared_ptr<Character> pChar = ToChar(_ctx.m_owner);

				SpherecastParam param;
				param.m_startPos = pChar->GetRoot()->localTransform.position;
				param.m_dir = Vector3(0.0f, -1.0f, 0.0f);
				param.m_maxDistance = 1.0f;
				param.m_radius = pChar->GetCapsuleRadius();

				RaycastResult result;
				bool bIsHit = _ctx.m_physics->SphereCast(param, result, Layer::Obstacle | Layer::Ground);
				if (bIsHit == false)
					return false;

				_ctx.m_pFirstObstacle = reinterpret_cast<Actor*>(result.GetActor());
				_ctx.m_firstObstacleHitPos = result.m_pos;
				_ctx.m_ledge = result.m_pos.y;
				_ctx.m_distance = result.m_distance;
				
				return true;
			}, 
			pOnHangingCheckObstacleTag,
			pEmpty
		);
		
			// 결론 노드-> 굳이 셀렉터를 써야하나?
			// Leaf 노드로 바꿔도 무관할 듯
			pOnHangingCheckObstacleTag->SetCondition(
				[](TravelContext& _ctx)
				{
					// 장애물 태그로 종류 판단
					uint8_t t = 0;
					bool bHasTag = _ctx.m_pFirstObstacle->GetTag().GetTagAt(TAG_ENV_DETAIL, t);
					if (bHasTag == false || t == (uint8_t)ETagEnvDetail::Default)
					{
						MG_LOG_INFO("[QueryTree] Hang to idle");
						_ctx.m_predictedActTag = (uint8_t)ETagAct::Wall_HangToIdle;
					}
					else
					{
						MG_LOG_INFO("[QueryTree] Hang to Beam");
						// 내려가는 절차이므로 stand로 통일
						_ctx.m_predictedActTag = (uint8_t)ETagAct::Beam_IdleToStand;
					}

					return 0;
				},
				{
					pReturn
				}
			);

		pOnHangingRight->SetCondition(
			[](TravelContext& _ctx)
			{
				_ctx.m_bIsRight = true; // 오른쪽 방향 설정

				// 캡슐 캐스트
				RaycastResult result;
				bool bIsHit = CheckSide(_ctx, result, _ctx.m_bIsRight, ToMask(Layer::Obstacle), MIN_OBSTACLE_DETECT_DIST);
				if (bIsHit) 
				{
					// 사이드에 캡슐을 쐈고, 히트한 상황
					_ctx.m_pFirstObstacle = reinterpret_cast<Actor*>(result.GetActor());
					_ctx.m_firstObstacleHitPos = result.m_pos;
					_ctx.m_distance = result.m_distance;
				}

				return bIsHit;
			},
			pOnSideDetected,		// 장애물 식별
			pOnSideEmpty			// 장애물이 없음 -> 전방으로 레이
		);

		pOnHangingLeft->SetCondition(
			[](TravelContext& _ctx) 
			{
				_ctx.m_bIsRight = false; // 왼쪽 방향 설정

				// 캡슐 캐스트
				RaycastResult result;
				bool bIsHit = CheckSide(_ctx, result, _ctx.m_bIsRight, ToMask(Layer::Obstacle), MIN_OBSTACLE_DETECT_DIST);
				if (bIsHit)
				{
					// 사이드에 캡슐을 쐈고, 히트한 상황
					_ctx.m_pFirstObstacle = reinterpret_cast<Actor*>(result.GetActor());
					_ctx.m_firstObstacleHitPos = result.m_pos;
					_ctx.m_distance = result.m_distance;
				}

				return bIsHit;
			}, 
			pOnSideDetected,		// 장애물 식별
			pOnSideEmpty			// 장애물이 없음 -> 전방으로 레이
		);

			// Right, Left 공통 처리 구간
			pOnSideDetected->SetCondition(
				[](TravelContext& _ctx) 
				{
					MG_LOG_INFO("[QueryTree] Check Side Obstacle Detected");

					// Beam 지형인 경우 처리
					uint8_t detailTag = 0;
					bool bHasTag = _ctx.m_pFirstObstacle->GetTag().GetTagAt(TAG_ENV_DETAIL, detailTag);
					if (bHasTag && detailTag == (uint8_t)ETagEnvDetail::Beam)
						return false; // beam 지형 처리로 전환

					// 장애물이 식별된 상황
					// Ledge 인지 확인 true : climb , false : inner rotate
					std::shared_ptr<Character> pChar = ToChar(_ctx.m_owner);
					const Transform& TF = pChar->GetRoot()->localTransform;
					const Vector3 DIR = _ctx.m_bIsRight ? TF.Right() : -TF.Right();

					RaycastResult result;
					bool bLedgeHit = CheckLedge(_ctx, 
						GetCharacterCenterPosition(_ctx),
						DIR,
						pChar->GetCapsuleRadius(),
						result);

					if (bLedgeHit) 
					{
						// climb
						_ctx.m_predictedActTag = (uint8_t)ETagAct::Wall_HangToMantle;
					}
					else
					{
						// outer rotate
						// 세부 판별이 더 필요할 수 있음
						_ctx.m_predictedActTag = (uint8_t)(_ctx.m_bIsRight ? ETagAct::Wall_OuterRotateRight : ETagAct::Wall_OuterRotateLeft);
					}

					// Ledge 확인 후 결과 반환
					return bLedgeHit;
				},
				pReturn,
				pCompareHeight
			);

			pOnSideEmpty->SetCondition(
				[](TravelContext& _ctx)
				{
					MG_LOG_INFO("[QueryTree] Side Obstacle is empty");

					// 이동하려고 예측하는 위치에서 전방으로 레이
					std::shared_ptr<Character> pChar = ToChar(_ctx.m_owner);
					const Transform& TF = pChar->GetRoot()->localTransform;
					const Vector3 POS = GetCharacterCenterPosition(_ctx) + 
						(_ctx.m_bIsRight ? 1 : -1) * MIN_OBSTACLE_DETECT_DIST * TF.Right();

					bool bIsHit = CapsuleCast(_ctx, POS, TF.Forward(), MIN_OBSTACLE_DETECT_DIST, ToMask(Layer::Obstacle));
					// true : 닿음 -> 여전히 벽 -> 별다른 액션 x
					if (bIsHit == false)
					{
						// false : 닿지 않음 회전 필요 inner 270 회전
						_ctx.m_predictedActTag = (uint8_t)(_ctx.m_bIsRight ? ETagAct::Wall_InnerRotateRight : ETagAct::Wall_InnerRotateLeft);
					}

					return bIsHit;
				},
				pEmpty,
				pReturn
			);

	
	pStateInAir->SetCondition(
		[](TravelContext& _ctx) 
		{ 
			std::shared_ptr<Character> pChar = ToChar(_ctx.m_owner);
			const Transform& TF = pChar->GetRoot()->localTransform;
			const Vector3 POS = GetCharacterCenterPosition(_ctx);

			// 떨어지는 중에 주변 장애물 탐색
			bool bFindObstacle = CheckObstacle(_ctx, POS, TF.Forward(), MAX_OBSTACLE_DETECT_DIST);

			if (bFindObstacle)
				pChar->TransitionStateMachine((uint8_t)Character::EState::Landing); // 낙하 상태 강제 종료

			return bFindObstacle;
		}, 
		pCheckObstacleTag,
		pEmpty
	);

	pStateBeam->SetCondition(
		[](TravelContext& _ctx) 
		{
			// 정면으로 먼저 확인
			std::shared_ptr<Character> pChar = ToChar(_ctx.m_owner);
			const Transform& TF = pChar->GetRoot()->localTransform;
			const Vector3 POS = GetCharacterCenterPosition(_ctx) + pChar->GetCapsuleRadius() * 2.0f * TF.Forward();

			return CheckObstacle(_ctx, POS, TF.Forward(), MAX_OBSTACLE_DETECT_DIST);
		},
		pCheckObstacleTag,
		pEmpty
	);

	return pRootQuery;
}