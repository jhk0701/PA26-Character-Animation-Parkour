#pragma once
#include "Scene/Component.h"
#include <functional>
#include <list>
#include "Physics/PhysicsWorld.h"

class Character;

namespace MiniEngine 
{
	namespace Physics { class PhysicsWorld; }

	struct TravelContext 
	{
		std::shared_ptr<Character> m_owner;
		std::shared_ptr<Physics::PhysicsWorld> m_physics;
		Physics::RaycastResult m_raycastResult;

		uint8_t m_predictedActTag;
	};

	struct TravelResult 
	{
		bool m_bIsEmpty{ true };
		Vector3 m_pos;
		uint8_t m_envTag; // 탐색한 결과 마주친 태그
		uint8_t m_actTag; // 탐색한 결과 취해야할 행동 태그
		void* m_pActor;
	};

	class NodeBase
	{
	public:
		virtual ~NodeBase() {};
		virtual TravelResult Execute(TravelContext& _context) = 0;
	};

	class ConditionNode : public NodeBase
	{
	public:
		void SetCondition(std::function<bool(TravelContext&)>&& _cond,
			std::shared_ptr<NodeBase> _nodeOnTrue, 
			std::shared_ptr<NodeBase> _nodeOnFalse) 
		{ 
			m_condition = _cond; 
			
			m_child.resize(2);
			m_child[0] = _nodeOnTrue;
			m_child[1] = _nodeOnFalse;
		}

		virtual TravelResult Execute(TravelContext& _context) override
		{
			if (!m_condition)
				return TravelResult();

			if (m_condition(_context))
				return m_child[0]->Execute(_context);
			else
				return m_child[1]->Execute(_context);
		};

	private:
		std::function<bool(TravelContext&)> m_condition;
		std::vector<std::shared_ptr<NodeBase>> m_child;
	};

	class LeafNode : public NodeBase 
	{
	public:
		virtual TravelResult Execute(TravelContext& _context) override
		{
			return TravelResult();
		};
	};

	class PerceptionComponent : public Component
	{
		enum Config
		{
			MAX_PERCEPTION_STEP = 8, // 총 몇번 레이를 쏘아 확인할 지
		};

	public:
		void OnAttach() override;
		void Tick(float _dt) override;
		void StartTravel(const Vector3& _moveDir); // 탐색
		void ConstructConditionTree();

		bool CheckByUnit(const TravelContext& _context, float _yOffset);

	private:
		std::weak_ptr<Physics::PhysicsWorld> m_physics;
		float m_unit{ 1.0f }; // 탐색 단위
		Vector3 m_ownerDir;

		// 1. 평지 이동 시, 장애물 탐색
		// 탐색 범위 
		float m_maxObsDist{ 2.0f };
		float m_maxLandDist{ 1000.0f }; // 바닥 탐색

		std::shared_ptr<NodeBase> m_QueryTree;
		std::list<TravelResult> m_travelResult;

		void Travel();
	};
}