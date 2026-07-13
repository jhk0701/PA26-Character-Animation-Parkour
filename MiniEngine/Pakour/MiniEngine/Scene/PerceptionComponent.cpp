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
		std::shared_ptr<ConditionNode> pIsClimbable = std::make_shared<ConditionNode>(); // 장애물을 넘을 수 있는지
		std::shared_ptr<ConditionNode> pObstableIsLandable = std::make_shared<ConditionNode>(); // 장애물을 넘을 수 있는지
		std::shared_ptr<ConditionNode> pIsHanging = std::make_shared<ConditionNode>();

		std::shared_ptr<LeafNode> pContinue = std::make_shared<LeafNode>(); // 무응답 -> 탐색 계속 신호
		std::shared_ptr<LeafNode> pSetLanding = std::make_shared<LeafNode>(); // 캐릭터를 Landing 상태로 전환
		std::shared_ptr<LeafNode> pSetHanging = std::make_shared<LeafNode>(); // 캐릭터를 Hanging 상태로 전환
		std::shared_ptr<LeafNode> pSetInAir = std::make_shared<LeafNode>(); // 캐릭터를 InAir 상태로 전환

		pRootQuery->SetCondition(
			[](TravelContext& _ctx) 
			{ 
				if (_ctx.m_owner)
					return _ctx.m_owner->GetCharState() == Character::EState::Landing;
				else
					return false; 
			},
			pFindObstacle,
			pIsHanging
		);

		pFindObstacle->SetCondition(
			[this](TravelContext& _ctx)
			{
				RaycastParam rayParam;
				rayParam.m_origin = _ctx.m_owner->GetRoot()->localTransform.position + Vector3(0.0f, 1.0f, 0.0f);
				rayParam.m_dir = _ctx.m_owner->GetRoot()->localTransform.Forward();
				rayParam.m_maxDistance = m_maxObsDist;

				return _ctx.m_physics->Raycast(rayParam, _ctx.m_raycastResult, ToMask(Layer::Obstacle));
			},
			pIsClimbable,	// 찾은 경우 오를(넘을) 수 있는지 확인
			pContinue		// 찾지 못한 경우 continue return 
		);

		pIsClimbable->SetCondition(
			[this](TravelContext& _ctx) 
			{ 
				MG_LOG_INFO("Check Obstable Is Climbable");
				
				bool bHitOnOneUnit = CheckByUnit(_ctx, m_unit);
				bool bHitOnTwoUnit = CheckByUnit(_ctx, m_unit * 2.0f);

				if (bHitOnOneUnit == false || 
					bHitOnTwoUnit == false)
				{
					if (bHitOnOneUnit == false)
						_ctx.m_predictedActTag = static_cast<uint8_t>(ETagAct::Vault);
					else if (bHitOnTwoUnit == false)
						_ctx.m_predictedActTag = static_cast<uint8_t>(ETagAct::Mantle);

					return true;
				}

				return false;
			},
			pSetLanding,
			pSetHanging
		);

		// pIsHanging 처리
		m_QueryTree = pRootQuery;
	}

	bool PerceptionComponent::CheckByUnit(const TravelContext& _context, float _unitAmount)
	{
		RaycastParam rayParam;
		rayParam.m_dir = _context.m_owner->GetRoot()->localTransform.Forward();
		rayParam.m_origin = _context.m_raycastResult.m_pos + Vector3(0.0f, 1.0f, 0.0f) * _unitAmount;
		rayParam.m_maxDistance = m_unit;
		RaycastResult rayResult;

		return _context.m_physics->Raycast(rayParam, rayResult, ToMask(Layer::Obstacle));
	}

	void PerceptionComponent::Tick(float _dt)
	{
		Component::Tick(_dt);
	}

	void PerceptionComponent::StartTravel(const Vector3& _moveDir)
	{
		if (m_physics.expired())
			return;

		m_ownerDir = _moveDir;
		m_travelResult.clear();

		Travel();
	}

	void PerceptionComponent::Travel()
	{
		TravelContext context;
		context.m_owner = std::dynamic_pointer_cast<Character>(owner.lock());
		context.m_physics = owner.lock()->GetScene()->GetPhysics().lock();

		m_QueryTree->Execute(context);
	}
	
}