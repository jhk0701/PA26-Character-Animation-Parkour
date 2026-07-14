#include "pch.h"
#include "Scene/PerceptionComponent.h"
#include "Content/Character.h"

#include "Scene/Scene.h"
#include "Physics/PhysicsWorld.h"
#include "Core/Log.h"
#include "Content/ContentConfig.h"

using namespace MiniEngine::Physics;
using namespace Content::Config;

namespace MiniEngine 
{
	void PerceptionComponent::OnAttach()
	{
		Component::OnAttach();

		m_physics = owner.lock()->GetScene()->GetPhysics();
	
		ConstructConditionTree();
	}

	void PerceptionComponent::ConstructConditionTree()
	{
		std::shared_ptr<ConditionNode> pRootQuery = std::make_shared<ConditionNode>();
		std::shared_ptr<ConditionNode> pFindObstacle = std::make_shared<ConditionNode>(); // 장애물 찾기
		std::shared_ptr<ConditionNode> pIsClimbableFirst = std::make_shared<ConditionNode>(); // 장애물을 넘을 수 있는지
		std::shared_ptr<ConditionNode> pIsClimbableSecond = std::make_shared<ConditionNode>(); // 장애물을 넘을 수 있는지
		std::shared_ptr<ConditionNode> pObstableIsLandable = std::make_shared<ConditionNode>(); // 장애물을 너머가 평지인지 확인
		std::shared_ptr<ConditionNode> pIsHanging = std::make_shared<ConditionNode>();
		std::shared_ptr<LeafNode> pContinue = std::make_shared<LeafNode>(); // 무응답 -> 탐색 계속 신호
		std::shared_ptr<LeafNode> pReturn = std::make_shared<LeafNode>(); // 결과 리턴
		std::shared_ptr<LeafNode> pSetHanging = std::make_shared<LeafNode>(); // 캐릭터를 Hanging 상태로 전환

		// 1번 확인 : 평지에 있는 상황인지
		pRootQuery->SetCondition(
			[](TravelContext& _ctx) 
			{ 
				if (_ctx.m_owner)
					return _ctx.m_owner->GetCharState() == Character::EState::Landing;
				
				return false; 
			},
			pFindObstacle,
			pIsHanging
		);
			// 평지에 있는데, 장애물을 발견했는지
			pFindObstacle->SetCondition(
				[this](TravelContext& _ctx)
				{
					CapsulecastParam capParam;
					capParam.m_startPos = _ctx.m_owner->GetRoot()->localTransform.position + Vector3(0.0f, 1.0f, 0.0f);
					capParam.m_startRot = Quaternion(0.0f, 0.0f, 0.0f, 1.0f);
					capParam.m_radius = _ctx.m_owner->GetCapsuleRadius();
					capParam.m_halfHeight = _ctx.m_owner->GetCapsuleHalfHeight();
					capParam.m_dir = _ctx.m_owner->GetRoot()->localTransform.Forward();
					capParam.m_maxDistance = m_maxObsDist;

					return _ctx.m_physics->CapsuleCast(capParam, _ctx.m_raycastResult, ToMask(Layer::Obstacle));
				},
				pIsClimbableFirst,	// 찾은 경우 오를(넘을) 수 있는지 확인
				pContinue		// 찾지 못한 경우 continue return 
			);
				// 장애물을 오를 수 있는지
				pIsClimbableFirst->SetCondition(
					[this](TravelContext& _ctx) 
					{ 
						MG_LOG_INFO("Check Obstable Is Climbable");
						if (CheckClimbableByUnit(_ctx, m_unit) == false)
							return true; // 1단위 넘을 수 있는 높이

						_ctx.m_units = 1;

						return false; // 2단위 체크 시도
					},
					pObstableIsLandable,	// 넘을 수 있음
					pIsClimbableSecond		// 넘을 수 없음. 다음 단위 체크
				);

				pIsClimbableSecond->SetCondition(
					[this](TravelContext& _ctx) 
					{
						// 첫번째 체크에서 true였다면 +y축으로 m_unit만큼 올려둠
						if (CheckClimbableByUnit(_ctx, m_unit) == false)
							return true; // 2단위로 넘을 수 있는 높이

						_ctx.m_units = 2;

						return false; // 2회 단위 체크에도 끝이 보이지 않음 -> 매달려야함
					},
					pObstableIsLandable,
					pSetHanging
				);

					pObstableIsLandable->SetCondition(
						[this](TravelContext& _ctx) 
						{
							MG_LOG_INFO("Check Obstable Is Landable");
							bool bIsLandable = CheckLandableOnObstacle(_ctx);

							_ctx.m_predictedActTag = static_cast<uint8_t>(bIsLandable ? 
								ETagAct::Mantle : ETagAct::Vault) + _ctx.m_units;

							return bIsLandable;
						}, 
						pReturn,
						pReturn
					);

						pReturn->SetTask([](TravelContext& _context) 
							{
								TravelResult result;
								result.m_bIsEmpty = false;
								result.m_actTag = _context.m_predictedActTag;
								result.m_pActor = _context.m_raycastResult.GetActor();

								return result;
							}
						);

		// pIsHanging 처리
		pIsHanging->SetCondition(
			[](TravelContext& _ctx) 
			{
				if (_ctx.m_owner)
					return _ctx.m_owner->GetCharState() == Character::EState::Hanging;
				
				return false;
			},
			pContinue, // TODO : 매달렸을 때 처리
			pContinue);

		m_QueryTree = pRootQuery;
	}

	bool PerceptionComponent::CheckClimbableByUnit(TravelContext& _context, float _unitAmount)
	{
		CapsulecastParam capParam;
		capParam.m_startPos = _context.m_raycastResult.m_pos + Vector3(0.0f, 1.0f, 0.0f) * _unitAmount;
		capParam.m_startRot = Quaternion(0.0f, 0.0f, 0.0f, 1.0f);
		capParam.m_dir = _context.m_owner->GetRoot()->localTransform.Forward();
		capParam.m_radius = _context.m_owner->GetCapsuleRadius();
		capParam.m_halfHeight = _context.m_owner->GetCapsuleHalfHeight();
		capParam.m_maxDistance = m_unit;

		RaycastResult rayResult;
		bool bIsHit = _context.m_physics->CapsuleCast(capParam, rayResult, ToMask(Layer::Obstacle));
		
		// 1 단위만큼 높여서 테스트했지만 장애물이 닿았음
		// 체크한 단위를 저장
		if (bIsHit)
			_context.m_raycastResult.m_pos = capParam.m_startPos;
		else // 넘을 수 있었음 -> 진행방향으로 1단위만큼 이동
			_context.m_raycastResult.m_pos = capParam.m_startPos + _context.m_owner->GetRoot()->localTransform.Forward() * m_unit;

		return bIsHit;
	}

	bool PerceptionComponent::CheckLandableOnObstacle(TravelContext& _context)
	{
		// climbing 테스트를 완료하고 호출될 것
		
		// 아래방향을 향해 레이캐스트
		RaycastParam rayParam;
		rayParam.m_origin = _context.m_raycastResult.m_pos;
		rayParam.m_dir = Vector3(0.0f, -1.0f, 0.0f);
		rayParam.m_maxDistance = m_maxLandDist;
		
		RaycastResult rayResult;
		return _context.m_physics->Raycast(rayParam, rayResult, ToMask(static_cast<Layer>(Layer::Obstacle)));
	}

	void PerceptionComponent::Tick(float _dt)
	{
		Component::Tick(_dt);

	}

	TravelResult PerceptionComponent::Travel()
	{
		if (m_physics.expired())
			return TravelResult();

		TravelContext context;
		context.m_owner = std::dynamic_pointer_cast<Character>(owner.lock());
		context.m_physics = owner.lock()->GetScene()->GetPhysics().lock();
		context.m_units = 0;

		return m_QueryTree->Execute(context);
	}
	
}