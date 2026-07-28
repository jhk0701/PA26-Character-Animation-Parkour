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
	// Config
	constexpr float MIN_OBSTACLE_DETECT_DIST = 1.0f;
	constexpr float MAX_OBSTACLE_DETECT_DIST = 2.5f;
	constexpr float CAPSULE_CONTACT_OFFSET = 0.2f;

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

	bool CheckLedge(TravelContext& _context, const Vector3& _pos, const Vector3& _dir, const float _radius, RaycastResult& _outResult)
	{
		SpherecastParam sphParam;
		sphParam.m_startPos = _pos;
		sphParam.m_dir = _dir;
		sphParam.m_radius = _radius;
		sphParam.m_maxDistance = MIN_OBSTACLE_DETECT_DIST;

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

			_context.m_pFirstObstacle = ToIObstacle(r.GetActor());
			_context.m_firstObstacleHitPos = r.m_pos;
			_context.m_firstObstacleHitNrm = r.m_nrm;
			_context.m_distance = r.m_distance;
			_context.m_ledge = r.m_pos.y; // 높이는 MeasureObstacleHeight 가 다시 잰다

			return true;
		}

		return false; // 딛고 선 장애물 외에 아무것도 없음 -> 찾지 못한 것
	}

	// 장애물의 높이를 캐릭터 발 기준 1.0m 밴드로 스캔한다.
	//
	// 전방 캡슐 스윕의 접촉점은 y 를 믿을 수 없다(세운 캡슐 x 벽면 = 접촉이 선분이라 y 가 구현 정의).
	// 그래서 x/z 만 가져다 쓰고 y 는 캐릭터 발에서 다시 쌓아 올린다.
	// 반지름 0.5 구는 [c-0.5, c+0.5] 를 덮으므로 스텝 1.0 이면 밴드가 정확히 접한다(틈/중복 없음).
	// 스윕이 eMTD 라 초기 겹침도 히트로 잡히므로, 짧게 쏘는 전방 캐스트가 사실상 오버랩 테스트가 된다.
	//
	// 반환 = 처음으로 비어 있던 밴드 인덱스
	//   0                   : 꼭대기가 발 높이 이하 (CCT stepOffset 이 처리할 턱)
	//   1 ~ MAX_BAND-1      : 넘거나 오를 수 있는 높이
	//   MAX_BAND            : 3.0m 이상 = 벽
	uint8_t MeasureObstacleHeight(TravelContext& _context, const Vector3& _dir)
	{
		std::shared_ptr<Character> pChar = ToChar(_context.m_owner);
		const float FOOT_Y = pChar->GetRoot()->localTransform.position.y;
		const Vector3& PROBE_XZ = _context.m_firstObstacleHitPos;

		uint8_t band = 0;
		for (; band < HEIGHT_PROBE_MAX_BAND; ++band)
		{
			SpherecastParam param;
			param.m_startPos = Vector3(
				PROBE_XZ.x,
				FOOT_Y + HEIGHT_PROBE_RADIUS + band * HEIGHT_PROBE_STEP,
				PROBE_XZ.z);
			param.m_radius = HEIGHT_PROBE_RADIUS;
			param.m_dir = _dir;
			param.m_maxDistance = HEIGHT_PROBE_FORWARD;

			RaycastResult result;
			if (_context.m_physics->SphereCast(param, result, ToMask(Layer::Obstacle)) == false)
				break; // 이 밴드는 비었다 -> 꼭대기가 이 아래
		}

		// 밴드 band 에서 처음 비었으므로 실제 꼭대기는 (FOOT_Y+(band-1)*STEP, FOOT_Y+band*STEP] 안에 있다.
		// 이 값이 베지어 착지 목표 Y 로 직행하므로 상한을 쓴다
		// (과대추정 = 위에서 떨어짐, 과소추정 = 장애물에 박힘)
		_context.m_ledge = FOOT_Y + band * HEIGHT_PROBE_STEP;

		// authored ObstacleLedge 볼륨이 있으면 정확한 꼭대기 y 로 덮어쓴다.
		// 마지막으로 히트한 밴드의 프로브 중심에서 쏘면 위 불확실 구간과 구가 정확히 겹친다.
		if (band > 0)
		{
			const Vector3 LEDGE_ORIGIN(PROBE_XZ.x, _context.m_ledge - HEIGHT_PROBE_RADIUS, PROBE_XZ.z);

			RaycastResult ledgeResult;
			CheckLedge(_context, LEDGE_ORIGIN, _dir, HEIGHT_PROBE_RADIUS, ledgeResult);
		}

		return band;
	}

	// 꼭대기에서 전방으로 0.5 씩 나아가며 하향 레이로 딛을 면이 이어지는지 잰다.
	// 결과는 {0, 0.5, 1.0}
	void MeasureObstacleDepth(TravelContext& _context, const Vector3& _dir)
	{
		const Vector3 TOP(
			_context.m_firstObstacleHitPos.x,
			_context.m_ledge + DEPTH_PROBE_LIFT,
			_context.m_firstObstacleHitPos.z);

		float depth = 0.0f;
		for (uint8_t i = 1; i <= DEPTH_PROBE_MAX_STEP; ++i)
		{
			RaycastParam param;
			param.m_origin = TOP + _dir * (DEPTH_PROBE_STEP * i);
			param.m_dir = Vector3(0.0f, -1.0f, 0.0f);
			param.m_maxDistance = DEPTH_PROBE_DOWN_DIST;

			RaycastResult result;
			if (_context.m_physics->Raycast(param, result, ToMask(Layer::Obstacle)) == false)
				break; // 구멍 -> 여기서부터는 딛을 수 없다

			// 마스크가 Obstacle-only 라 지면은 안 맞지만 뒤에 있는 다른 장애물은 맞는다.
			// 그걸 깊이로 세면 얇은 난간이 Mantle 로 새므로 액터 동일성으로 막는다
			if (ToIObstacle(result.GetActor()) != _context.m_pFirstObstacle)
				break;

			depth = DEPTH_PROBE_STEP * i;
		}

		_context.m_depth = depth;
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
	std::shared_ptr<SelectorNode> pCheckHeight = std::make_shared<SelectorNode>(); // 장애물 높이 측정 -> 무시 / 깊이측정 / 벽
	std::shared_ptr<ConditionNode> pMeasureDepth = std::make_shared<ConditionNode>(); // 딛을 면의 깊이 측정

	// Obstacle Beam
	std::shared_ptr<ConditionNode> pBeamCompareHeight = std::make_shared<ConditionNode>(); // 캐릭터와 장애물의 y 위치 비교

	// Obstacle Protrude
	std::shared_ptr<ConditionNode> pProtrudeCheckLedge = std::make_shared<ConditionNode>();

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

				return CheckObstacle(_ctx, POS, TF.Forward(), MAX_OBSTACLE_DETECT_DIST, 1.0f, true);
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

			// 높이를 재고 세 갈래로 나눈다. 액션 태그 결정은 여기서 하지 않는다(소비처 담당)
			pCheckHeight->SetCondition(
				[](TravelContext& _ctx)
				{
					const Vector3& FWD = ToChar(_ctx.m_owner)->GetRoot()->localTransform.Forward();
					const uint8_t BAND = MeasureObstacleHeight(_ctx, FWD);

					if (BAND == 0)
						return (uint8_t)0; // 꼭대기가 발보다 낮다 -> CCT stepOffset 이 처리할 턱

					if (BAND >= HEIGHT_PROBE_MAX_BAND)
						return (uint8_t)2; // 3.0m 이상 -> 벽. ledge 값만 넘기고 소비처가 Wall 로 판정

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
					param.m_maxDistance = MIN_OBSTACLE_DETECT_DIST;
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
					param.m_startPos = _ctx.m_firstObstacleHitPos - TF.Forward();
					param.m_radius = pChar->GetCapsuleRadius();
					param.m_maxDistance = MIN_OBSTACLE_DETECT_DIST;

					RaycastResult result;
					bool bIsHit = _ctx.m_physics->SphereCast(param, result, ToMask(Layer::Obstacle));
					if (bIsHit == false)
					{
						// MG_LOG_INFO("[QueryTree] Can detour");
						// _ctx.m_predictedActTag = (uint8_t)ETagAct::Wall_HangToMantle;

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
						_ctx.m_pFirstObstacle = ToIObstacle(result.GetActor());
						_ctx.m_firstObstacleHitPos = result.m_pos;
						_ctx.m_distance = result.m_distance;
						_ctx.m_ledge = result.m_pos.y;

						// MG_LOG_INFO("[QueryTree] Wall Ledge Climb");
						// _ctx.m_predictedActTag = (uint8_t)ETagAct::Wall_HangToMantle;
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
				if (_ctx.m_physics->SphereCast(param, result, Layer::Obstacle | Layer::Ground) == false)
					return false;

				_ctx.m_pFirstObstacle = ToIObstacle(result.GetActor());
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
					bool bHasTag = _ctx.m_pFirstObstacle->TryGetTag(TAG_ENV_DETAIL, t);
					if (bHasTag == false || t == (uint8_t)ETagEnvDetail::Default)
					{
						MG_LOG_INFO("[QueryTree] Hang to idle");
						// _ctx.m_predictedActTag = (uint8_t)ETagAct::Wall_HangToIdle;
					}
					else
					{
						MG_LOG_INFO("[QueryTree] Hang to Beam");
						// 내려가는 절차이므로 stand로 통일
						// _ctx.m_predictedActTag = (uint8_t)ETagAct::Beam_IdleToStand;
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
				// 캡슐 캐스트
				RaycastResult result;
				bool bIsHit = CheckSide(_ctx, result, true, ToMask(Layer::Obstacle), MIN_OBSTACLE_DETECT_DIST);
				if (bIsHit) 
				{
					// 사이드에 캡슐을 쐈고, 히트한 상황
					_ctx.m_pFirstObstacle = ToIObstacle(result.GetActor());
					_ctx.m_firstObstacleHitPos = result.m_pos;
					_ctx.m_firstObstacleHitNrm = result.m_nrm;
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
				// 캡슐 캐스트
				RaycastResult result;
				bool bIsHit = CheckSide(_ctx, result, false, ToMask(Layer::Obstacle), MIN_OBSTACLE_DETECT_DIST);
				if (bIsHit)
				{
					// 사이드에 캡슐을 쐈고, 히트한 상황
					_ctx.m_pFirstObstacle = ToIObstacle(result.GetActor());
					_ctx.m_firstObstacleHitPos = result.m_pos;
					_ctx.m_firstObstacleHitNrm = result.m_nrm;
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
					bool bHasTag = _ctx.m_pFirstObstacle->TryGetTag(TAG_ENV_DETAIL, detailTag);
					if (bHasTag && detailTag == (uint8_t)ETagEnvDetail::Beam)
						return false; // beam 지형 처리로 전환

					// 장애물이 식별된 상황
					// Ledge 인지 확인 true : climb , false : inner rotate
					std::shared_ptr<Character> pChar = ToChar(_ctx.m_owner);
					const Transform& TF = pChar->GetRoot()->localTransform;
					const Vector3 DIR = TF.Right(); // _ctx.m_bIsRight ? TF.Right() : -TF.Right();

					RaycastResult result;
					bool bLedgeHit = CheckLedge(_ctx, 
						GetCharacterCenterPosition(_ctx),
						DIR,
						pChar->GetCapsuleRadius(),
						result);

					if (bLedgeHit) 
					{
						// climb
						// _ctx.m_predictedActTag = (uint8_t)ETagAct::Wall_HangToMantle;
					}
					else
					{
						// outer rotate
						// 세부 판별이 더 필요할 수 있음
						// _ctx.m_predictedActTag = (uint8_t)(_ctx.m_bIsRight ? ETagAct::Wall_OuterRotateRight : ETagAct::Wall_OuterRotateLeft);
					}

					// Ledge 확인 후 결과 반환
					return bLedgeHit;
				},
				pReturn,
				pBeamCompareHeight
			);

			pOnSideEmpty->SetCondition(
				[](TravelContext& _ctx)
				{
					MG_LOG_INFO("[QueryTree] Side Obstacle is empty");

					// 이동하려고 예측하는 위치에서 전방으로 레이
					std::shared_ptr<Character> pChar = ToChar(_ctx.m_owner);
					const Transform& TF = pChar->GetRoot()->localTransform;

					CapsulecastParam param;
					param.m_startPos = GetCharacterCenterPosition(_ctx) + TF.Right(); //  (_ctx.m_bIsRight ? 1 : -1)* MIN_OBSTACLE_DETECT_DIST* TF.Right();
					param.m_dir = TF.Forward();
					param.m_halfHeight = pChar->GetCapsuleHalfHeight();
					param.m_radius = pChar->GetCapsuleRadius();
					param.m_maxDistance = MIN_OBSTACLE_DETECT_DIST;

					RaycastResult result;
					bool bIsHit = _ctx.m_physics->CapsuleCast(param, result, ToMask(Layer::Obstacle));
					// true : 닿음 -> 여전히 벽 -> 별다른 액션 x
					if (bIsHit == false)
					{
						// false : 닿지 않음 회전 필요 inner 270 회전
						// _ctx.m_predictedActTag = (uint8_t)(_ctx.m_bIsRight ? ETagAct::Wall_InnerRotateRight : ETagAct::Wall_InnerRotateLeft);
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
			bool bFindObstacle = CheckObstacle(_ctx, POS, TF.Forward(), MAX_OBSTACLE_DETECT_DIST, 2.0f, true);

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

			return CheckObstacle(_ctx, POS, TF.Forward(), MAX_OBSTACLE_DETECT_DIST, 2.0f, true);
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