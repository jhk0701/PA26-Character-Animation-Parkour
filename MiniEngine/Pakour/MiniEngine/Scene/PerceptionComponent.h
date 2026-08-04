#pragma once
#include "Scene/Component.h"
#include "Scene/IObstacle.h"
#include "Physics/PhysicsWorld.h"
#include <functional>

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
		float m_distance{ 0.0f };
		float m_ledge{ 0.0f };
		float m_depth{ 0.0f };
	};

	struct TravelResult 
	{
		IObstacle* m_pFirstObstacle{ nullptr };	// 장애물 객체의 포인터
		Vector3 m_firstObstacleHitPos;			// 접촉 위치
		Vector3 m_firstObstacleHitNrm;			// 접촉 표면 노멀 벡터
		float m_obstacleDistance{ 0.0f };		// 캐릭터와 거리
		float m_obstacleLedge{ 0.0f };			// 모서리 (최종 높이)
		float m_obstacleDepth{ 0.0f };			// 깊이

		void Reset();
	};

	enum class EPerceptionResult : uint8_t
	{
		Succeess,
		Fail,
		
		END
	};

#pragma region Perception Nodes 인지 처리 노드

	// 최상위 부모
	// 상속해서 활용할 것
	class PerceptionNode
	{
	public:
		virtual ~PerceptionNode() {};
		virtual EPerceptionResult Execute(TravelContext& _context, TravelResult& _result) = 0;
	};

	// 자식 중 1개 실행
	class SelectorNode : public PerceptionNode
	{
	public:
		EPerceptionResult Execute(TravelContext& _context, TravelResult& _result) override;
		virtual uint8_t InvokeCondition(TravelContext& _context) = 0;

		void SetChildren(std::vector<std::shared_ptr<PerceptionNode>>&& _children);
		size_t GetChildrenCnt() const { return m_children.size(); }

	private:
		std::vector<std::shared_ptr<PerceptionNode>> m_children;
	};

	// 단순 이진 조건문 노드
	class ConditionNode : public PerceptionNode
	{
	public:
		EPerceptionResult Execute(TravelContext& _context, TravelResult& _result) override;
		virtual bool InvokeCondition(TravelContext& _context) = 0;

		void SetChildren(std::vector<std::shared_ptr<PerceptionNode>>&& _children);
		size_t GetChildrenCnt() const { return m_children.size(); }

	private:
		std::vector<std::shared_ptr<PerceptionNode>> m_children;
	};

	// 자식 연속 실행 노드
	class SequenceNode : public PerceptionNode
	{
	public:
		EPerceptionResult Execute(TravelContext& _context, TravelResult& _result) override;
		void SetChildren(std::vector<std::shared_ptr<PerceptionNode>>&& _children);

	private:
		std::vector<std::shared_ptr<PerceptionNode>> m_children;
	};

	// Leaf 노드 - 최종 작업 수행
	class TaskNode : public PerceptionNode
	{
	public:
		EPerceptionResult Execute(TravelContext& _context, TravelResult& _result) override;
		virtual EPerceptionResult InvokeTask(TravelContext& _context, TravelResult& _result) = 0;
	};

#pragma endregion

	class PerceptionComponent : public Component
	{
	public:
		void OnAttach() override;

		EPerceptionResult Travel(); // 탐색
		void SetQueryTree(std::shared_ptr<PerceptionNode>&& _newTree) { m_queryTree = _newTree; };
		bool IsInitialized() const { return m_queryTree != nullptr; };

		const TravelResult& GetLastestTravelResult() const { return m_result; }
	private:
		std::weak_ptr<Physics::PhysicsWorld> m_physics;
		std::shared_ptr<PerceptionNode> m_queryTree;

		TravelResult m_result; // 가장 마지막으로 인식한 결과물
	};
}