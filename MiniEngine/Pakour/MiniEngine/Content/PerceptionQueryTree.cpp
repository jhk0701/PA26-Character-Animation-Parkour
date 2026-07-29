#include "pch.h"
#include "Content/PerceptionQueryTree.h"
#include "Content/ContentConfig.h"
#include "Content/Character.h"
#include "Content/Obstacle.h"
#include "Scene/IObstacle.h"

#include "Scene/PerceptionComponent.h"
#include "Physics/PhysicsWorld.h"
#include "Core/Log.h"

using namespace MiniEngine;
using namespace MiniEngine::Physics;
using namespace Content::Config;


namespace 
{
	// 헬퍼 메서드 모음

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

	// 캐스트 결과를 컨텍스트로 옮긴다. 높이를 따로 재는 경로는 m_ledge 를 뒤에서 덮어쓴다
	// RaycastResult / HitResult 둘 다 받는다 (필드는 같은데 타입이 갈려 있음)
	template <typename THit>
	void FillFromResult(TravelContext& _context, const THit& _result)
	{
		_context.m_pFirstObstacle = ToIObstacle(_result.GetActor());
		_context.m_firstObstacleHitPos = _result.m_pos;
		_context.m_firstObstacleHitNrm = _result.m_nrm;
		_context.m_distance = _result.m_distance;
		_context.m_ledge = _result.m_pos.y;
	}

	bool CheckLedge(TravelContext& _context, const Vector3& _pos, const Vector3& _dir, const float _radius, RaycastResult& _outResult)
	{
		SpherecastParam sphParam;
		sphParam.m_startPos = _pos;
		sphParam.m_dir = _dir;
		sphParam.m_radius = _radius;
		sphParam.m_maxDistance = ToChar(_context.m_owner)->GetPerceptionConfig().minObstacleDetectDist;

		bool bIsHit = _context.m_physics->SphereCast(sphParam, _outResult, ToMask(Layer::ObstacleLedge));

		if (bIsHit)
			_context.m_ledge = _outResult.m_pos.y; // authored ledge = 정확한 꼭대기 y

		return bIsHit;
	}

	// 캐릭터 기준으로 현재 위치에서 특정 방향에 장애물이 있는지 체크
	// _bExcludeGroundActor : 지금 딛고 선 장애물(경사로 등 이미 올라온 것)을 후보에서 제외한다.
	//                        전방 탐지에만 의미가 있으므로 위/옆으로 쏘는 호출부는 끄고 쓸 것
	bool CheckObstacle(TravelContext& _context, const Vector3& _pos, const Vector3& _dir, const float _dist,
		const float _hMultiplier = 2.0f, const bool _bExcludeGroundActor = false)
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

		// 지금 딛고 선 액터를 찾아둔다 (없으면 nullptr -> 아래 순회가 자연히 첫 히트를 집는다)
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
			if (pGroundActor != nullptr && r.GetActor() == pGroundActor)
				continue; // 이미 올라온 장애물 -> 다음 후보로

			FillFromResult(_context, r); // 높이는 MeasureObstacleHeight 가 다시 잰다
			return true;
		}

		return false; // 딛고 선 장애물 외에 아무것도 없음 -> 찾지 못한 것
	}

	// 장애물 높이 측정
	// 횟수는 config를 통해서 조절
	uint8_t MeasureObstacleHeight(TravelContext& _context, const Vector3& _dir)
	{
		std::shared_ptr<Character> pChar = ToChar(_context.m_owner);
		const PerceptionConfig& CONFIG = pChar->GetPerceptionConfig();
		const float FOOT_Y = pChar->GetRoot()->localTransform.position.y;
		const Vector3& PROBE_XZ = _context.m_firstObstacleHitPos;

		uint8_t band = 0;
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

			RaycastResult result;
			if (_context.m_physics->SphereCast(param, result, ToMask(Layer::Obstacle)) == false)
				break; // 닿지 않음
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

	// 꼭대기에서 전방으로 0.5 씩 나아가며 하향 레이로 딛을 면이 이어지는지 잰다.
	// 결과는 {0, 0.5, 1.0}
	void MeasureObstacleDepth(TravelContext& _context, const Vector3& _dir)
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

	// 양쪽 사이드 확인 (매달린 상태의 좌우 탐색)
	bool CheckSide(TravelContext& _context, RaycastResult& _outResult, bool _bIsRight, uint32_t _layerMask, float _dist)
	{
		std::shared_ptr<Character> pChar = ToChar(_context.m_owner);
		const Transform& TF = pChar->GetRoot()->localTransform;

		SpherecastParam param;
		param.m_startPos = GetCharacterCenterPosition(_context);
		param.m_dir = _bIsRight ? TF.Right() : -TF.Right();
		param.m_maxDistance = _dist;
		param.m_radius = pChar->GetPerceptionConfig().onHangingSearchRadius;

		return _context.m_physics->SphereCast(param, _outResult, _layerMask);
	}
}

void PerceptionQueryTree::Init(const PerceptionConfig& _newConfig)
{
	m_config = _newConfig;
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
	std::shared_ptr<SelectorNode> pCheckHeight = std::make_shared<SelectorNode>(); // 장애물 높이 측정 -> 무시 / 깊이측정 / 벽
	std::shared_ptr<ConditionNode> pMeasureDepth = std::make_shared<ConditionNode>(); // 딛을 면의 깊이 측정

	// Obstacle Beam
	std::shared_ptr<ConditionNode> pBeamCompareHeight = std::make_shared<ConditionNode>(); // 캐릭터와 장애물의 y 위치 비교

	// Obstacle Protrude
	std::shared_ptr<ConditionNode> pProtrudeCheckLedge = std::make_shared<ConditionNode>();

	// InAir
	std::shared_ptr<ConditionNode> pStateInAir = std::make_shared<ConditionNode>();

	// Hanging — 입력 방향으로 2.0m 프로브 1발. 넷 다 { pReturn, pEmpty } 로 모양이 같다
	std::shared_ptr<SelectorNode> pStateHanging = std::make_shared<SelectorNode>();
	std::shared_ptr<ConditionNode> pOnHangingUp = std::make_shared<ConditionNode>();		// 올라설 벽 ledge
	std::shared_ptr<ConditionNode> pOnHangingDown = std::make_shared<ConditionNode>();	// 내려설 지면
	std::shared_ptr<ConditionNode> pOnHangingLeft = std::make_shared<ConditionNode>();	// 새 장애물
	std::shared_ptr<ConditionNode> pOnHangingRight = std::make_shared<ConditionNode>();	// 새 장애물

	// Beam
	std::shared_ptr<ConditionNode> pStateBeam = std::make_shared<ConditionNode>();

	// Protrude
	std::shared_ptr<SelectorNode> pStateProtrude = std::make_shared<SelectorNode>();

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
			result.m_pFirstObstacle = _context.m_pFirstObstacle;
			result.m_firstObstacleHitPos = _context.m_firstObstacleHitPos;
			result.m_firstObstacleHitNrm = _context.m_firstObstacleHitNrm;
			result.m_obstacleDistance = _context.m_distance;
			result.m_obstacleDepth = _context.m_depth;
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
			pStateBeam,
			pStateProtrude
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
				const float DIST = pChar->GetPerceptionConfig().maxObstacleDetectDist;

				return CheckObstacle(_ctx, POS, TF.Forward(), DIST, 1.0f, true);
			},
			pCheckObstacleTag,	// 찾은 경우 태그 확인
			pEmpty			// 찾지 못한 경우 empty return
		);

			pCheckObstacleTag->SetCondition(
				[](TravelContext& _ctx) 
				{
					uint8_t detailTag = 0;
					_ctx.m_pFirstObstacle->TryGetTag(TAG_ENV_DETAIL, detailTag);

					return detailTag;
				}, 
				{
					// ETagEnvDetail 순서
					pCheckHeight,			// Default // 일반 장애물 높이 확인
					pBeamCompareHeight,		// Beam
					pProtrudeCheckLedge		// Protrude 돌출부
				}
			);

			// 높이를 재고 세 갈래로 나눈다. 
			pCheckHeight->SetCondition(
				[](TravelContext& _ctx)
				{
					std::shared_ptr<Character> pChar = ToChar(_ctx.m_owner);
					const Vector3& FWD = pChar->GetRoot()->localTransform.Forward();
					const uint8_t BAND = MeasureObstacleHeight(_ctx, FWD);

					if (BAND == 0)
						return (uint8_t)0; // 꼭대기가 발보다 낮다 -> CCT stepOffset 이 처리할 턱

					if (BAND >= pChar->GetPerceptionConfig().maxHeightStep)
						return (uint8_t)2; // 3.0m 이상 -> 벽 판정

					return (uint8_t)1; // 넘거나 오를 수 있는 높이 -> 깊이 측정으로
				},
				{
					pEmpty,
					pMeasureDepth,
					pReturn
				}
			);

				pMeasureDepth->SetCondition(
					[](TravelContext& _ctx)
					{
						const Vector3& FWD = ToChar(_ctx.m_owner)->GetRoot()->localTransform.Forward();
						MeasureObstacleDepth(_ctx, FWD);

						MG_LOG_INFO("[QueryTree] ledge : {}, depth : {}", _ctx.m_ledge, _ctx.m_depth);
						return true;
					},
					pReturn,
					pReturn
				);

			pBeamCompareHeight->SetCondition(
				[](TravelContext& _ctx) 
				{
					// 장애물 y 위치 비교
					std::shared_ptr<Character> pChar = ToChar(_ctx.m_owner);
					IObstacle* pObs = _ctx.m_pFirstObstacle;

					SpherecastParam param;
					param.m_dir = pChar->GetRoot()->localTransform.Forward();
					param.m_startPos = _ctx.m_firstObstacleHitPos;
					param.m_radius = pChar->GetCapsuleRadius();
					param.m_maxDistance = 1.0f;
					RaycastResult result;
					bool bIsHit = _ctx.m_physics->SphereCast(param, result, ToMask(Layer::ObstacleLedge));

					// const Vector3& CHAR_POS = GetCharacterCenterPosition(_ctx);
					const Vector3& OBS_POS = pObs->GetTransform().position;
					// bool bStepable = OBS_POS.y <= CHAR_POS.y;
					// _ctx.m_predictedActTag = (uint8_t)(bStepable ? ETagAct::BeamStand : ETagAct::BeamHanging);
					_ctx.m_ledge = bIsHit ? result.m_pos.y : OBS_POS.y;

					/*
					const Vector3 DebugVec = { _ctx.m_firstObstacleHitPos.x, _ctx.m_ledge, _ctx.m_firstObstacleHitPos.z };
					MG_LOG_INFO("[QueryTree] Beam Obstacle is found : ({}, {}, {}) {}", DebugVec.x, DebugVec.y, DebugVec.z, bStepable ? "will step" : "will hang");
					*/
					return true;
				},
				pReturn,
				pReturn
			);

			pProtrudeCheckLedge->SetCondition(
				[](TravelContext& _ctx) 
				{
					std::shared_ptr<Character> pChar = ToChar(_ctx.m_owner);

					SpherecastParam param;
					param.m_dir = pChar->GetRoot()->localTransform.Forward();
					param.m_maxDistance = pChar->GetPerceptionConfig().maxObstacleDetectDist;
					param.m_radius = 0.5f;
					param.m_startPos = _ctx.m_firstObstacleHitPos;
					
					RaycastResult result;
					bool bIsHit = _ctx.m_physics->SphereCast(param, result, ToMask(Layer::ObstacleLedge));
					if (bIsHit) 
					{
						_ctx.m_ledge = result.m_pos.y;
						_ctx.m_firstObstacleHitNrm = result.m_nrm;

						// _ctx.m_predictedActTag = (uint8_t)ETagAct::Protrude;
					}

					return bIsHit; 
				},
				pReturn,
				pEmpty
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

		// 위 방향 탐색 -> 올라설 벽 ledge 를 찾기
		pOnHangingUp->SetCondition(
			[](TravelContext& _ctx)
			{
				std::shared_ptr<Character> pChar = ToChar(_ctx.m_owner);
				const PerceptionConfig& CONFIG = pChar->GetPerceptionConfig();

				SpherecastParam param;
				param.m_startPos = GetCharacterCenterPosition(_ctx);
				param.m_dir = Vector3(0.0f, 1.0f, 0.0f);
				param.m_radius = CONFIG.onHangingSearchRadius;
				param.m_maxDistance = CONFIG.onHangingSearchDist;

				RaycastResult result;
				if (_ctx.m_physics->SphereCast(param, result, ToMask(Layer::ObstacleLedge)) == false)
					return false;

				FillFromResult(_ctx, result);
				return true;
			},
			pReturn,
			pEmpty
		);

		// 아래 : 내려설 지면. 좁게 볼 필요가 없어 단순 레이로 충분하다
		// 지면(Ground)도 Obstacle 액터라 IObstacle 포인터가 유효하다
		pOnHangingDown->SetCondition(
			[](TravelContext& _ctx)
			{
				std::shared_ptr<Character> pChar = ToChar(_ctx.m_owner);

				RaycastParam param;
				param.m_origin = pChar->GetRoot()->localTransform.position;
				param.m_dir = Vector3(0.0f, -1.0f, 0.0f);
				param.m_maxDistance = pChar->GetPerceptionConfig().onHangingSearchDist;

				RaycastResult result;
				if (_ctx.m_physics->Raycast(param, result, Layer::Obstacle | Layer::Ground) == false)
					return false;

				FillFromResult(_ctx, result);
				return true;
			},
			pReturn,
			pEmpty
		);

		// 좌우 : 옮겨갈 새 장애물 탐색. 종류 판단과 상태 전환은 HangingState 담당
		pOnHangingRight->SetCondition(
			[](TravelContext& _ctx)
			{
				const float DIST = ToChar(_ctx.m_owner)->GetPerceptionConfig().onHangingSearchDist;
				RaycastResult result;
				if (CheckSide(_ctx, result, true, ToMask(Layer::Obstacle), DIST) == false)
					return false;

				FillFromResult(_ctx, result);
				return true;
			},
			pReturn,
			pEmpty
		);

		pOnHangingLeft->SetCondition(
			[](TravelContext& _ctx)
			{
				const float DIST = ToChar(_ctx.m_owner)->GetPerceptionConfig().onHangingSearchDist;

				RaycastResult result;
				if (CheckSide(_ctx, result, false, ToMask(Layer::Obstacle), DIST) == false)
					return false;

				FillFromResult(_ctx, result);
				return true;
			},
			pReturn,
			pEmpty
		);

	
	pStateInAir->SetCondition(
		[](TravelContext& _ctx) 
		{ 
			std::shared_ptr<Character> pChar = ToChar(_ctx.m_owner);
			const Transform& TF = pChar->GetRoot()->localTransform;
			const Vector3 POS = GetCharacterCenterPosition(_ctx);
			const float DIST = pChar->GetPerceptionConfig().maxObstacleDetectDist;

			// 떨어지는 중에 주변 장애물 탐색
			bool bFindObstacle = CheckObstacle(_ctx, POS, TF.Forward(), DIST, 2.0f, true);

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
			const float DIST = pChar->GetPerceptionConfig().maxObstacleDetectDist;

			return CheckObstacle(_ctx, POS, TF.Forward(), DIST, 2.0f, true);
		},
		pCheckObstacleTag,
		pEmpty
	);

	// 키 입력 기준 상하좌우 방향 탐색
	pStateProtrude->SetCondition(
		[](TravelContext& _ctx)
		{
			return 0;
		}, 
		{
			pEmpty	
		}
	);

	return pRootQuery;
}

