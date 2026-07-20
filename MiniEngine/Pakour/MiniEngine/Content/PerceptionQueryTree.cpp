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
			return pChar->GetState() == (Character::EState)_state;

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

			MG_LOG_INFO("[QueryTree] Hit Pos : ({}, {}, {})", _context.m_firstObstacleHitPos.x, _context.m_firstObstacleHitPos.y, _context.m_firstObstacleHitPos.z);
		}

		return bIsHit;
	}

	bool CapsuleCast(TravelContext& _context, const Vector3& _pos, const Vector3& _dir) 
	{
		std::shared_ptr<Character> pChar = ToChar(_context.m_owner);

		CapsulecastParam capParam;
		capParam.m_startPos = _pos;
		capParam.m_dir = _dir;
		capParam.m_radius = pChar->GetCapsuleRadius();
		capParam.m_halfHeight = pChar->GetCapsuleHalfHeight();
		capParam.m_maxDistance = 1.0f;

		RaycastResult result;
		return _context.m_physics->CapsuleCast(capParam, result, ToMask(Layer::Obstacle));
	}

	bool CheckVaultable(TravelContext& _context, const Vector3& _initPos, const Vector3& _dir, const float _unit, uint8_t _maxCnt = 2)
	{
		const Vector3 up = Vector3(0.0f, 1.0f, 0.0f) * _unit;
		Vector3 rayPosition = _initPos; 

		for (uint8_t i = 0; i < _maxCnt; ++i)
		{
			rayPosition += up;
			
			bool bIsHit = CapsuleCast(_context, rayPosition, _dir);
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

	bool CheckLedge(TravelContext& _context, const Vector3& _pos, float _radius, RaycastResult& _outResult)
	{
		std::shared_ptr<Character> pChar = ToChar(_context.m_owner);
		
		SpherecastParam sphParam;
		sphParam.m_startPos = _pos;
		sphParam.m_dir = pChar->GetRoot()->localTransform.Forward();
		sphParam.m_radius = _radius;
		sphParam.m_maxDistance = 1.0f;

		bool bIsHit = _context.m_physics->SphereCast(sphParam, _outResult, ToMask(Layer::ObstacleLedge));

		// MG_LOG_INFO("[QueryTree] LedgeFind Cast Origin : ({}, {}, {})", sphParam.m_startPos.x, sphParam.m_startPos.y, sphParam.m_startPos.z);

		if (bIsHit)
		{
			_context.m_ledge = _outResult.m_pos.y;
			MG_LOG_INFO("[QueryTree] Ledge Found : ({}, {}, {})", _outResult.m_pos.x, _outResult.m_pos.y, _outResult.m_pos.z);
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
	bool CheckHangableOnSide(TravelContext& _context, bool _bIsRight, float _dist)
	{
		std::shared_ptr<Character> pChar = ToChar(_context.m_owner);
		
		const Transform& tf

		CapsulecastParam param;
		param.m_dir = _bIsRight ? pChar->GetRoot()->localTransform.Right() : -pChar->GetRoot()->localTransform.Right();
		param.m_radius = pChar->GetCapsuleRadius();
		param.m_halfHeight = pChar->GetCapsuleHalfHeight();
		param.m_startPos = pChar
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
	std::shared_ptr<ConditionNode> pObstableIsLandable = std::make_shared<ConditionNode>(); // 장애물을 너머가 평지인지 확인

	std::shared_ptr<ConditionNode> pIsHanging = std::make_shared<ConditionNode>();
	std::shared_ptr<SelectorNode> pOnHanging = std::make_shared<SelectorNode>();
	std::shared_ptr<ConditionNode> pOnHangingClimbable = std::make_shared<ConditionNode>();
	std::shared_ptr<ConditionNode> pOnHangingLandable = std::make_shared<ConditionNode>();
	std::shared_ptr<ConditionNode> pOnHangingLeft = std::make_shared<ConditionNode>();
	std::shared_ptr<ConditionNode> pOnHangingRight = std::make_shared<ConditionNode>();

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
			[](TravelContext& _ctx) 
			{ 
				return CheckObstacle(_ctx); 
			},
			pCheckHeight,	// 찾은 경우 높이 확인
			pEmpty			// 찾지 못한 경우 empty return 
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
					_ctx.m_predictedActTag = (uint8_t)Content::Config::ETagAct::IdleToHang;
					return false;
				}

				// ledge 체크
				const float RADIUS = CHAR_H * 0.25f;

				Vector3 rayPosition = _ctx.m_raycastPos;
				rayPosition.y += (_ctx.m_units - 1) * CHAR_H;

				// 상반신 크기 확인
				RaycastResult result;
				if (CheckLedge(_ctx, rayPosition + Vector3(0.0f, 1.0f, 0.0f) * RADIUS, RADIUS, result))
					return bVaultable;

				// 하반신 크기 확인
				if (CheckLedge(_ctx, rayPosition - Vector3(0.0f, 1.0f, 0.0f) * RADIUS, RADIUS, result))
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
						ETagAct::Mantle : ETagAct::Vault) + _ctx.m_units;

					if (bIsLandable)
						MG_LOG_INFO("[QueryTree] Mantle + {}", _ctx.m_units);
					else
						MG_LOG_INFO("[QueryTree] Vault + {}", _ctx.m_units);

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
		pOnHanging,				// 매달렸을 때 처리
		pEmpty					// false는 공중에서 떨어지는 상태일 것 // TODO : 허공상태일 때 인식하려면 여기서 이어서 작업
	);

		pOnHanging->SetCondition(
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
				pOnHangingClimbable,
				pOnHangingLandable,
				pOnHangingRight,
				pOnHangingLeft,
				pEmpty
			}
		);

		pOnHangingClimbable->SetCondition(
			[](TravelContext& _ctx) 
			{
				std::shared_ptr<Character> pChar = ToChar(_ctx.m_owner);
				const float CHAR_H = GetCharHeight(_ctx);
				const float RADIUS = 0.25f;
				Vector3 pos = pChar->GetRoot()->localTransform.position + Vector3(0.0f, CHAR_H * 1.5f, 0.0f);

				// MG_LOG_INFO("[QueryTree] Check Ledge");
				RaycastResult result;
				bool bIsHit = CheckLedge(_ctx, pos, RADIUS, result);
				if (bIsHit) 
				{
					_ctx.m_firstObstacle = result.GetActor();
					_ctx.m_firstObstacleHitPos = result.m_pos;
					_ctx.m_distance = result.m_distance;
					_ctx.m_ledge = result.m_pos.y;
					_ctx.m_predictedActTag = (uint8_t)ETagAct::HangToMantle;
				}

				return bIsHit;
			},
			pReturn,
			pEmpty
		);

		pOnHangingLandable->SetCondition(
			[](TravelContext& _ctx)
			{
				bool bIsHit = CheckLandable(
					_ctx, 
					ToChar(_ctx.m_owner)->GetRoot()->localTransform.position, 
					Layer::Obstacle | Layer::Ground, 
					1.0f); // 1m 아래 확인

				if (bIsHit)
				{
					MG_LOG_INFO("[QueryTree] Hang to idle");
					_ctx.m_predictedActTag = (uint8_t)ETagAct::HangToIdle;
				}

				return bIsHit;
			},
			pReturn,
			pEmpty
		);

		pOnHangingRight->SetCondition(
			[](TravelContext& _ctx)
			{


				return true;
			},
			pReturn,
			pEmpty
		);

		pOnHangingLeft->SetCondition(
			[](TravelContext& _ctx) 
			{
				return true;
			}, 
			pReturn, 
			pEmpty
		);
	
	return pRootQuery;
}