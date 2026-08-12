#pragma once
#include "Scene/Component.h"
#include "Perception/Interface/IObstacle.h"
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

		// TODO : 중복된 사항 -> 정리 후 제거할 것
		IObstacle* m_pFirstObstacle{ nullptr };
		Vector3 m_firstObstacleHitPos;
		Vector3 m_firstObstacleHitNrm;
		float m_distance{ 0.0f };
		float m_ledge{ 0.0f };
		float m_depth{ 0.0f };
		bool m_bDetectLedge{ false };
	};

	struct TravelResult 
	{
		IObstacle* m_pFirstObstacle{ nullptr };	// 장애물 객체의 포인터
		Vector3 m_firstObstacleHitPos;			// 접촉 위치
		Vector3 m_firstObstacleHitNrm;			// 접촉 표면 노멀 벡터
		float m_obstacleDistance{ 0.0f };		// 캐릭터와 거리
		float m_obstacleLedge{ 0.0f };			// 모서리 (최종 높이)
		float m_obstacleDepth{ 0.0f };			// 깊이
		bool m_bDetectLedge{ false };			// 모서리 탐지 여부

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
	// 노드에 붙어 실행 전 조건문 역할을 할 것
	class PerceptionDecorator
	{
	public:
		virtual ~PerceptionDecorator() {};
		virtual bool Evaluate(const TravelContext& _context) const = 0;
	};

	// 최상위 부모
	// 상속해서 활용할 것
	class PerceptionNode
	{
	public:
		virtual ~PerceptionNode() {};
		virtual EPerceptionResult Execute(TravelContext& _context, TravelResult& _result) = 0;

		// 단일 노드 내, 다중 조건은 기본 && 로 처리
		// 한 노드에 조건 두 개 넣은건, 둘 다 통과해야한다고 의도한 것으로 간주함
		bool Evaluate(const TravelContext& _context) const;
		void SetConditions(std::vector<std::shared_ptr<PerceptionDecorator>>&& _conditions) { m_conditions = std::move(_conditions); };

	private:
		std::vector<std::shared_ptr<PerceptionDecorator>> m_conditions;
	};

	// Leaf 노드 - 최종 작업 수행
	// Context Write는 오로지 Task에서만 수행하기
	class TaskNode : public PerceptionNode
	{
	public:
		EPerceptionResult Execute(TravelContext& _context, TravelResult& _result) override;
		virtual EPerceptionResult InvokeTask(TravelContext& _context, TravelResult& _result) = 0;
	};

	// 합성 노드, 자식을 가질 수 있는 형태 노드에서 상속
	// 여전히 순수가상함수 상태
	class CompositeNode : public PerceptionNode
	{
	public:
		void SetChildren(std::vector<std::shared_ptr<PerceptionNode>>&& _children) { m_children = std::move(_children); };

	protected:
		size_t GetChildrenCnt() const { return m_children.size(); }
		const std::vector<std::shared_ptr<PerceptionNode>>& GetChildren() const { return m_children; }

	private:
		std::vector<std::shared_ptr<PerceptionNode>> m_children;
	};

	// 자식 연속 실행 노드
	// 하나라도 Fail이면 중단
	class SequenceNode : public CompositeNode
	{
	public:
		EPerceptionResult Execute(TravelContext& _context, TravelResult& _result) override;
	};

	// 자식 중 1개 실행
	// 조건 확인 후, True인 자식을 실행
	class SelectorNode : public CompositeNode 
	{
	public:
		EPerceptionResult Execute(TravelContext& _context, TravelResult& _result) override;
	};


	// Switch 조건문 노드
	class SwitchNode : public CompositeNode
	{
	public:
		EPerceptionResult Execute(TravelContext& _context, TravelResult& _result) override;
		virtual uint8_t InvokeCondition(TravelContext& _context) = 0;
	};

	// 단순 이진 조건문 노드
	class BinaryConditionNode : public CompositeNode
	{
	public:
		EPerceptionResult Execute(TravelContext& _context, TravelResult& _result) override;
		virtual bool InvokeCondition(TravelContext& _context) = 0;
	};

#pragma endregion

	class PerceptionComponent : public Component
	{
	public:
		EPerceptionResult Travel(); // 탐색

		void SetQueryTree(std::shared_ptr<PerceptionNode>&& _newTree) { m_queryTree = _newTree; };

		bool IsInitialized() const { return m_queryTree != nullptr; };

		const TravelResult& GetLastestTravelResult() const { return m_result; }

	private:
		std::shared_ptr<PerceptionNode> m_queryTree;

		TravelResult m_result; // 가장 마지막으로 인식한 결과물
	};
}