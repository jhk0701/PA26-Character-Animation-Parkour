#pragma once
#include "Scene/Component.h"

namespace MiniEngine
{
	struct TravelResult;

	struct ProcessContext
	{
		std::shared_ptr<Actor> pOwner;
	};

#pragma region Process Condition

	// 조건들의 최상위 부모
	// 상속해서 활용할 것
	class ProcessCondition
	{
	public:
		virtual ~ProcessCondition() {};
		bool Process(const TravelResult& _result, const ProcessContext& _context);

		// 결과 반전 처리 -> 조건문에서 ! 와 동일
		void Invert(bool _bIsInvert) { m_bIsInvert = _bIsInvert; }

	protected:
		// 자식에서 세부 구현
		virtual bool Evaluate(const TravelResult& _result, const ProcessContext& _context) const = 0;

	private:
		bool m_bIsInvert{ false };
	};

	class CompositeCondition : public ProcessCondition
	{
	public:
		void SetChildren(std::vector<std::shared_ptr<ProcessCondition>>&& _children);

	protected:
		std::vector<std::shared_ptr<ProcessCondition>> m_children;
	};

	// 자식 조건들이 모두 만족해야 true
	class ConditionAnd : public CompositeCondition
	{
	protected:
		bool Evaluate(const TravelResult& _result, const ProcessContext& _context) const override;
	};

	// 자식 조건 중 1개만 만족하면 true. 이후 조건은 return
	class ConditionOr : public CompositeCondition
	{
	protected:
		bool Evaluate(const TravelResult& _result, const ProcessContext& _context) const override;
	};

	// 조건 모음
	class ProcessData
	{
	public:
		void Init(uint8_t _result, std::shared_ptr<ProcessCondition> _pCondition);
		bool TryQuery(const TravelResult& _inResult, const ProcessContext& _inContext, uint8_t& _outResult) const;

	private:
		uint8_t m_result;
		std::shared_ptr<ProcessCondition> m_pCondition;
	};

#pragma endregion

	class ProcessorComponent : public Component
	{
	public:
		bool ProcessResult(const TravelResult& _inTravelResult, uint8_t& _outResult) const; // 탐색 결과를 주어진 조건 데이터에 맞게 처리
		void SetProcessData(std::vector<std::shared_ptr<ProcessData>>&& _datas) 
		{
			m_processDatas = std::move(_datas);
		};

	private:
		std::vector<std::shared_ptr<ProcessData>> m_processDatas;
	};
}