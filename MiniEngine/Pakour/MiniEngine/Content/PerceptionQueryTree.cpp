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
	constexpr float MAX_OBSTACLE_DETECT_DIST = 2.0f;

	std::shared_ptr<Character> ToChar(std::shared_ptr<Actor> _actor) 
	{
		return std::dynamic_pointer_cast<Character>(_actor);
	}

	float GetCharHeight(TravelContext& _context)
	{
		std::shared_ptr<Character> pChar = ToChar(_context.m_owner);

		if (!pChar)
			return 1.0f;
		
		return pChar->GetCapsuleHalfHeight() * 2.0f; // 캡슐 원본 높이 -> 일반적으로 캐릭터의 높이 맞게 캡슐의 높이가 결정될 것
	}

	bool CheckOwnerState(TravelContext& _context, uint8_t _state)
	{
		if (std::shared_ptr<Character> pChar = ToChar(_context.m_owner))
			return pChar->GetCharState() == (Character::EState)_state;

		return false;
	}

	bool CheckObstacle(TravelContext& _context) 
	{
		std::shared_ptr<Character> pChar = ToChar(_context.m_owner);
		const float charHalfHeight = pChar->GetCapsuleHalfHeight();

		CapsulecastParam capParam;
		capParam.m_startPos = pChar->GetRoot()->localTransform.position + Vector3(0.0f, 1.0f, 0.0f) * charHalfHeight;
		capParam.m_radius = pChar->GetCapsuleRadius();
		capParam.m_halfHeight = charHalfHeight;
		capParam.m_dir = _context.m_owner->GetRoot()->localTransform.Forward();
		capParam.m_maxDistance = MAX_OBSTACLE_DETECT_DIST;

		// MG_LOG_INFO("[QueryTree] : Check Obstacle : ({}, {}, {})", capParam.m_startPos.x, capParam.m_startPos.y, capParam.m_startPos.z);
		RaycastResult result;

		bool bIsHit = _context.m_physics->CapsuleCast(capParam, result, ToMask(Layer::Obstacle));
		if (bIsHit)
		{
			_context.m_raycastPos = result.m_pos;
			_context.m_firstObstacle = result.GetActor();
			_context.m_firstObstacleHitPos = result.m_pos;
			_context.m_distance = result.m_distance;

			_context.m_units = 1; // 1 단위 확정
			_context.m_ledge = result.m_pos.y;

			// MG_LOG_INFO("[QueryTree] : Hit : ({}, {}, {})", _ctx.m_raycastPos.x, _ctx.m_raycastPos.y, _ctx.m_raycastPos.z);
		}

		return bIsHit;
	}

	bool CapsuleCast(TravelContext& _context, const Vector3& _pos) 
	{
		std::shared_ptr<Character> pChar = ToChar(_context.m_owner);

		CapsulecastParam capParam;
		capParam.m_startPos = _pos;
		capParam.m_dir = pChar->GetRoot()->localTransform.Forward();
		capParam.m_radius = pChar->GetCapsuleRadius();
		capParam.m_halfHeight = pChar->GetCapsuleHalfHeight();
		capParam.m_maxDistance = 1.0f;

		RaycastResult result;
		return _context.m_physics->CapsuleCast(capParam, result, ToMask(Layer::Obstacle));
	}

	bool CheckVaultable(TravelContext& _context, const Vector3& _initPos, const float _unit, uint8_t _maxCnt = 2)
	{
		const Vector3 up = Vector3(0.0f, 1.0f, 0.0f) * _unit;
		Vector3 rayPosition = _initPos; 
		for (uint8_t i = 0; i < _maxCnt; ++i)
		{
			rayPosition += up;

			bool bIsHit = CapsuleCast(_context, rayPosition);
			if (bIsHit)
			{
				// MG_LOG_INFO("[QueryTree] Obstacle hit on : ({}, {}, {}), unit : {}", rayPosition.x, rayPosition.y, rayPosition.z, _context.m_units);

				// 이 높이에선 아직 닿음
				_context.m_units++; // 단위 상승
				_context.m_ledge = rayPosition.y; // 대략적인 위치만 기입
			}
			else
			{
				// MG_LOG_INFO("[QueryTree] Obstacle valutable : ({}, {}, {}), unit : {}", rayPosition.x, rayPosition.y, rayPosition.z, _context.m_units);
				return true; // 이 높이에선 닿지 않음 -> 넘어갈 수 있음
			}
		}

		return false; // 매달려야 함
	}

	bool CheckLedge(TravelContext& _context, const Vector3& _pos, float _radius)
	{
		std::shared_ptr<Character> pChar = ToChar(_context.m_owner);
		
		SpherecastParam sphParam;
		sphParam.m_startPos = _pos;
		sphParam.m_dir = pChar->GetRoot()->localTransform.Forward();
		sphParam.m_radius = _radius;
		sphParam.m_maxDistance = 1.0f;

		RaycastResult result;
		bool bIsHit = _context.m_physics->SphereCast(sphParam, result, ToMask(Layer::ObstacleLedge));

		// MG_LOG_INFO("[QueryTree] LedgeFind Cast Origin : ({}, {}, {})", sphParam.m_startPos.x, sphParam.m_startPos.y, sphParam.m_startPos.z);

		if (bIsHit)
		{
			_context.m_ledge = result.m_pos.y;
			// MG_LOG_INFO("[QueryTree] Ledge Found : {}", _context.m_ledge);
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

	bool CheckFront(TravelContext& _context, uint32_t _layerMask, float _dist)
	{
		RaycastParam rayParam;
		rayParam.m_origin = _context.m_raycastPos;
		rayParam.m_dir = Vector3(0.0f, 0.0f, 1.0f);
		rayParam.m_maxDistance = _dist;

		RaycastResult result;
		return _context.m_physics->Raycast(rayParam, result, _layerMask);
	}
}

// 콘텐츠에서 사용할 지형 인식 로직
// TODO: 데이터 객체로 정리할 것
std::shared_ptr<QueryNodeBase> PerceptionQueryTree::ConstructTree()
{
	// Condition
	std::shared_ptr<ConditionNode> pRootQuery = std::make_shared<ConditionNode>();
	std::shared_ptr<ConditionNode> pFindObstacle = std::make_shared<ConditionNode>(); // 장애물 찾기
	std::shared_ptr<ConditionNode> pCheckHeight = std::make_shared<ConditionNode>(); // 장애물 높이 확인

	// 레거시
	std::shared_ptr<ConditionNode> pIsClimbableFirst = std::make_shared<ConditionNode>();	// 장애물을 넘을 수 있는지 1 단위
	std::shared_ptr<ConditionNode> pIsClimbableSecond = std::make_shared<ConditionNode>();	// 장애물을 넘을 수 있는지 2 단위
	std::shared_ptr<ConditionNode> pObstableIsLandable = std::make_shared<ConditionNode>(); // 장애물을 너머가 평지인지 확인

	std::shared_ptr<ConditionNode> pIsHanging = std::make_shared<ConditionNode>();
	std::shared_ptr<ConditionNode> pCheckAroundOnHaning = std::make_shared<ConditionNode>();

	// Leaf
	std::shared_ptr<LeafNode> pEmpty = std::make_shared<LeafNode>(); // 빈 결과 리턴, 탐색 계속 신호
	std::shared_ptr<LeafNode> pReturn = std::make_shared<LeafNode>(); // 결과 리턴

	pReturn->SetTask(
		[](TravelContext& _context)
		{
			TravelResult result;
			result.m_bIsEmpty = false;
			result.m_actTag = _context.m_predictedActTag;
			result.m_pFirstObstacle = _context.m_firstObstacle;
			result.m_firstObstacleHitPos = _context.m_firstObstacleHitPos;
			result.m_distanceObstacle = _context.m_distance;
			result.m_obstacleLedge = _context.m_ledge;
			return result;
		}
	);
	
	// 루트 확인 : 평지에 있는 상황인지
	pRootQuery->SetCondition(
		[this](TravelContext& _ctx) 
		{ return CheckOwnerState(_ctx, (uint8_t)Character::EState::Landing); },
		pFindObstacle,
		pIsHanging
	);
		// 평지에 있는데, 장애물을 발견했는지
		pFindObstacle->SetCondition(
			[](TravelContext& _ctx) { return CheckObstacle(_ctx); },
			pCheckHeight,	// 찾은 경우 높이 확인
			pEmpty			// 찾지 못한 경우 empty return 
		);

		pCheckHeight->SetCondition(
			[](TravelContext& _ctx) 
			{
				// 장애물의 높이 확인
				const float CHAR_H = GetCharHeight(_ctx);
				bool bVaultable = CheckVaultable(_ctx, _ctx.m_raycastPos, CHAR_H);

				if (bVaultable == false)
				{
					_ctx.m_predictedActTag = (uint8_t)Content::Config::ETagAct::IdleToHang;
					return false;
				}

				// ledge 체크
				const float RADIUS = CHAR_H * 0.25f;

				Vector3 rayPosition = _ctx.m_raycastPos;
				rayPosition.y += (_ctx.m_units - 1) * CHAR_H;

				// 상반신 크기 확인
				if (CheckLedge(_ctx, rayPosition + Vector3(0.0f, 1.0f, 0.0f) * RADIUS, RADIUS))
					return bVaultable;

				// 하반신 크기 확인
				if (CheckLedge(_ctx, rayPosition - Vector3(0.0f, 1.0f, 0.0f) * RADIUS, RADIUS))
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
					const float CHAR_H = GetCharHeight(_ctx);

					Vector3 rayPosition = _ctx.m_raycastPos;
					rayPosition.y += _ctx.m_units * CHAR_H;
					rayPosition += ToChar(_ctx.m_owner)->GetRoot()->localTransform.Forward() * 1.0f;

					bool bIsLandable = CheckLandable(_ctx, rayPosition, ToMask(Layer::Obstacle), CHAR_H);

					// 원래는 여기서 Vault / Hurdle 중에 갈려야함
					// 모션이 제한적이라 우선 Mantle, Vault를 사용
					_ctx.m_predictedActTag = static_cast<uint8_t>(bIsLandable ?
						ETagAct::Mantle : ETagAct::Vault);

					if (bIsLandable)
						MG_LOG_INFO("[QueryTree] Mantle");
					else
						MG_LOG_INFO("[QueryTree] Vault");

					return bIsLandable;
				},
				pReturn,
				pReturn
			);


	// pIsHanging 처리
	pIsHanging->SetCondition(
		[this](TravelContext& _ctx)
		{
			return CheckOwnerState(_ctx, (uint8_t)Character::EState::Hanging);
		},
		pCheckAroundOnHaning,	// 매달렸을 때 처리
		pEmpty					// false는 공중에서 떨어지는 상태일 것 // TODO : 허공상태일 때 인식하려면 여기서 이어서 작업
	);

		pCheckAroundOnHaning->SetCondition(
			[this](TravelContext& _ctx)
			{
				// 벽면에 있는 상황
				std::shared_ptr<Character> pChar = ToChar(_ctx.m_owner);
				Vector2 inputDir = pChar->GetInputDir();

				_ctx.m_raycastPos = pChar->GetRoot()->localTransform.position;
				const float charHeight = GetCharHeight(_ctx);

				//if (inputDir.y < 0 &&
				//	CheckLandable(_ctx, Layer::Obstacle | Layer::Ground, charHeight))
				//{
				//	// 아래를 향하는데
				//	// 1 단위만큼 거리에 바닥이나 착지할 수 있는 장애물이 있음
				//	_ctx.m_predictedActTag = (uint8_t)ETagAct::HangToIdle;
				//	_ctx.m_firstObstacle = _ctx.m_raycastResult.GetActor();
				//	_ctx.m_firstObstacleHitPos = _ctx.m_raycastResult.m_pos;
				//	_ctx.m_distance = _ctx.m_raycastResult.m_distance;

				//	MG_LOG_INFO("Character Hang To Idle");
				//	return true;
				//}

				/*if (inputDir.y > 0 &&
					CheckClimbableByUnit(_ctx, charHeight * 1.5f) == false)
				{
					_ctx.m_raycastPos = pChar->GetRoot()->localTransform.position;
					if (CheckFront(_ctx, ToMask(Layer::Obstacle), MAX_OBSTACLE_DETECT_DIST))
					{
						_ctx.m_firstObstacle = _ctx.m_raycastResult.GetActor();
						_ctx.m_firstObstacleHitPos = _ctx.m_raycastResult.m_pos;
						_ctx.m_distance = _ctx.m_raycastResult.m_distance;
					}

					_ctx.m_predictedActTag = (uint8_t)ETagAct::HangToMantle;

					MG_LOG_INFO("Character Hang To Mantle");
					return true;
				}*/

				// 좌우 양옆을 확인
				// 갈 곳이 있음 해당 부분으로 이동

				return false;
			},
			pReturn,
			pEmpty
		);
	
	return pRootQuery;
}