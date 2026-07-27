#pragma once
#include "Scene/Component.h"
#include "Scene/IObstacle.h"
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
		IObstacle* m_pFirstObstacle{ nullptr };
		Vector3 m_firstObstacleHitPos;
		Vector3 m_firstObstacleHitNrm;
		uint8_t m_predictedActTag;
		uint8_t m_units;
		Vector3 m_raycastPos;
		float m_distance{ 0.0f };
		float m_ledge{ 0.0f };
		bool m_bIsRight{ false };
	};

	struct TravelResult 
	{
		bool m_bIsEmpty{ true };
		uint8_t m_actTag; // 탐색한 결과 취해야할 행동 태그
		uint8_t m_units;
		
		IObstacle* m_pFirstObstacle{ nullptr };
		float m_distanceObstacle{ 0.0f };
		Vector3 m_firstObstacleHitPos;
		Vector3 m_firstObstacleHitNrm;
		float m_obstacleLedge{ 0.0f };

		void Reset();
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
		std::vector<std::shared_ptr<QueryNodeBase>> m_children;
	};

	class SelectorNode : public QueryNodeBase 
	{
	public:
		void SetCondition(std::function<uint8_t(TravelContext&)>&& _cond,
			std::vector<std::shared_ptr<QueryNodeBase>>&& _results);

		TravelResult Execute(TravelContext& _context) override;

	private:
		std::function<uint8_t(TravelContext&)> m_condition;
		std::vector<std::shared_ptr<QueryNodeBase>> m_children;
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

		void Travel();// 탐색
		void SetQueryTree(std::shared_ptr<QueryNodeBase>&& _newTree) { m_queryTree = _newTree; };
		bool IsInitialized() const { return m_queryTree != nullptr; };

		const TravelResult& GetLastestTravelResult() const { return m_result; }

	private:
		std::weak_ptr<Physics::PhysicsWorld> m_physics;
		std::shared_ptr<QueryNodeBase> m_queryTree;
		TravelResult m_result; // 가장 마지막으로 인식한 데이터
	};
}