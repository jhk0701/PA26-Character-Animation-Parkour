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
	constexpr float MAX_LAND_DETECT_DIST = 1000.0f;
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

	bool CheckClimbableByUnit(TravelContext& _context, float _unitAmount)
	{
		std::shared_ptr<Character> pChar = ToChar(_context.m_owner);

		CapsulecastParam capParam;
		capParam.m_startPos = _context.m_raycastPos + Vector3(0.0f, 1.0f, 0.0f) * _unitAmount;
		capParam.m_startRot = Quaternion(0.0f, 0.0f, 0.0f, 1.0f);
		capParam.m_dir = pChar->GetRoot()->localTransform.Forward();
		capParam.m_radius = pChar->GetCapsuleRadius();
		capParam.m_halfHeight = pChar->GetCapsuleHalfHeight();
		capParam.m_maxDistance = 1.0f;

		return _context.m_physics->CapsuleCast(capParam, _context.m_raycastResult, ToMask(Layer::Obstacle));
	}

	bool CheckLandable(TravelContext& _context, uint32_t _layerMask, float _dist)
	{
		// climbing 테스트를 완료하고 호출될 것
		// 아래방향을 향해 레이캐스트
		RaycastParam rayParam;
		rayParam.m_origin = _context.m_raycastPos;
		rayParam.m_dir = Vector3(0.0f, -1.0f, 0.0f);
		rayParam.m_maxDistance = _dist;

		return _context.m_physics->Raycast(rayParam, _context.m_raycastResult, _layerMask);
	}

	bool CheckFront(TravelContext& _context, uint32_t _layerMask, float _dist)
	{
		RaycastParam rayParam;
		rayParam.m_origin = _context.m_raycastPos;
		rayParam.m_dir = Vector3(0.0f, 0.0f, 1.0f);
		rayParam.m_maxDistance = _dist;

		return _context.m_physics->Raycast(rayParam, _context.m_raycastResult, _layerMask);
	}
}

// 콘텐츠에서 사용할 지형 인식 로직
// TODO: 데이터 객체로 정리할 것
std::shared_ptr<QueryNodeBase> PerceptionQueryTree::ConstructTree()
{
	// Condition
	std::shared_ptr<ConditionNode> pRootQuery = std::make_shared<ConditionNode>();
	std::shared_ptr<ConditionNode> pFindObstacle = std::make_shared<ConditionNode>(); // 장애물 찾기
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
			[this](TravelContext& _ctx)
			{
				std::shared_ptr<Character> pChar = ToChar(_ctx.m_owner);

				CapsulecastParam capParam;
				capParam.m_startPos = pChar->GetRoot()->localTransform.position + Vector3(0.0f, 1.0f, 0.0f) * pChar->GetCapsuleHalfHeight();
				capParam.m_startRot = Quaternion(0.0f, 0.0f, 0.0f, 1.0f);
				capParam.m_radius = pChar->GetCapsuleRadius();
				capParam.m_halfHeight = pChar->GetCapsuleHalfHeight();
				capParam.m_dir = _ctx.m_owner->GetRoot()->localTransform.Forward();
				capParam.m_maxDistance = MAX_OBSTACLE_DETECT_DIST;

				bool bIsHit = _ctx.m_physics->CapsuleCast(capParam, _ctx.m_raycastResult, ToMask(Layer::Obstacle));
				_ctx.m_raycastPos = _ctx.m_raycastResult.m_pos;

				if (bIsHit)
				{
					_ctx.m_firstObstacle = _ctx.m_raycastResult.GetActor();
					_ctx.m_firstObstacleHitPos = _ctx.m_raycastResult.m_pos;
					_ctx.m_distance = _ctx.m_raycastResult.m_distance;
				}

				return bIsHit;
			},
			pIsClimbableFirst,	// 찾은 경우 오를(넘을) 수 있는지 확인
			pEmpty		// 찾지 못한 경우 empty return 
		);

			// 장애물을 오를 수 있는지
			pIsClimbableFirst->SetCondition(
				[this](TravelContext& _ctx)
				{
					MG_LOG_INFO("Check Obstable Is Climbable");
					const float charHeight = GetCharHeight(_ctx);
					if (CheckClimbableByUnit(_ctx, charHeight) == false)
					{
						// 넘을 수 있었음 -> 1단위 넘을 수 있는 높이
						// 진행방향으로 1단위만큼 이동
						_ctx.m_raycastPos += 
							Vector3(0.0f, 1.0f, 0.0f) * charHeight + 
							_ctx.m_owner->GetRoot()->localTransform.Forward() * MAX_OBSTACLE_DETECT_DIST;

						return true;
					}

					// 1 단위만큼 높여서 테스트했지만 장애물이 닿았음
					_ctx.m_raycastPos += Vector3(0.0f, 1.0f, 0.0f) * charHeight;
					_ctx.m_units = 1;

					return false; // 2단위 체크 시도
				},
				pObstableIsLandable,	// 넘을 수 있음
				pIsClimbableSecond		// 넘을 수 없음. 다음 단위 체크
			);

				pIsClimbableSecond->SetCondition(
					[this](TravelContext& _ctx)
					{
						const float charHeight = GetCharHeight(_ctx);

						// 첫번째 체크에서 true였다면 +y축으로 m_unit만큼 올려둠
						if (CheckClimbableByUnit(_ctx, charHeight) == false)
						{
							// 2단위로 넘을 수 있는 높이
							_ctx.m_raycastPos += 
								Vector3(0.0f, 1.0f, 0.0f) * charHeight + 
								_ctx.m_owner->GetRoot()->localTransform.Forward() * 1.0f;
							return true;
						}

						_ctx.m_units = 2;
						_ctx.m_predictedActTag = (uint8_t)ETagAct::IdleToHang;
						return false; // 2회 단위 체크에도 끝이 보이지 않음 -> 매달려야함
					},
					pObstableIsLandable,
					pReturn
				);

					pObstableIsLandable->SetCondition(
						[this](TravelContext& _ctx)
						{
							MG_LOG_INFO("Check Obstable Is Landable");
							
							bool bIsLandable = CheckLandable(_ctx, ToMask(Layer::Obstacle), MAX_LAND_DETECT_DIST);

							_ctx.m_predictedActTag = static_cast<uint8_t>(bIsLandable ?
								ETagAct::Mantle : ETagAct::Vault) + _ctx.m_units;

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

				if (inputDir.y < 0 &&
					CheckLandable(_ctx, Layer::Obstacle | Layer::Ground, charHeight))
				{
					// 아래를 향하는데
					// 1 단위만큼 거리에 바닥이나 착지할 수 있는 장애물이 있음
					_ctx.m_predictedActTag = (uint8_t)ETagAct::HangToIdle;
					_ctx.m_firstObstacle = _ctx.m_raycastResult.GetActor();
					_ctx.m_firstObstacleHitPos = _ctx.m_raycastResult.m_pos;
					_ctx.m_distance = _ctx.m_raycastResult.m_distance;

					MG_LOG_INFO("Character Hang To Idle");
					return true;
				}

				if (inputDir.y > 0 &&
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
				}

				return false;
			},
			pReturn,
			pEmpty
		);
	
	return pRootQuery;
}