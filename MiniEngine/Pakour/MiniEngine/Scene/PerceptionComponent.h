#pragma once
#include "Scene/Component.h"
#include <functional>
#include "Physics/PhysicsWorld.h"

namespace MiniEngine 
{
	namespace Physics { class PhysicsWorld; }
	class Actor;

	struct TravelContext 
	{
		std::shared_ptr<Actor> m_owner;
		std::shared_ptr<Physics::PhysicsWorld> m_physics;
		Physics::RaycastResult m_raycastResult;
		Vector3 m_raycastPos;
		uint8_t m_predictedActTag;
		uint8_t m_units;
	};

	struct TravelResult 
	{
		bool m_bIsEmpty{ true };
		Vector3 m_pos;
		uint8_t m_actTag; // 탐색한 결과 취해야할 행동 태그
		void* m_pActor;
	};

	class QueryNodeBase
	{
	public:
		virtual ~QueryNodeBase() {};
		virtual TravelResult Execute(TravelContext& _context) = 0;
	};

	class ConditionNode : public QueryNodeBase
	{
	public:
		void SetCondition(std::function<bool(TravelContext&)>&& _cond,
			std::shared_ptr<QueryNodeBase> _nodeOnTrue,
			std::shared_ptr<QueryNodeBase> _nodeOnFalse);

		TravelResult Execute(TravelContext& _context) override;

	private:
		std::function<bool(TravelContext&)> m_condition;
		std::vector<std::shared_ptr<QueryNodeBase>> m_child;
	};

	class LeafNode : public QueryNodeBase
	{
	public:
		void SetTask(std::function<TravelResult(TravelContext&)>&& _newTask) { m_task = _newTask; }

		TravelResult Execute(TravelContext& _context) override;

	private:
		std::function<TravelResult(TravelContext&)> m_task;
	};

	class PerceptionComponent : public Component
	{
	public:
		void OnAttach() override;

		TravelResult Travel();// 탐색
		void SetQuertTree(std::shared_ptr<QueryNodeBase>&& _newTree) { m_queryTree = _newTree; };
		bool IsInitialized() const { return m_queryTree != nullptr; };

	private:
		std::weak_ptr<Physics::PhysicsWorld> m_physics;
		float m_unit{ 1.0f }; // 탐색 단위
		Vector3 m_ownerDir;

		// 1. 평지 이동 시, 장애물 탐색
		// 탐색 범위 
		float m_maxObsDist{ 1.0f };
		float m_maxLandDist{ 1000.0f }; // 바닥 탐색

		std::shared_ptr<QueryNodeBase> m_queryTree;

	};
}