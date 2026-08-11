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
		void Reset() { m_bIsProcessed = false; }

#ifdef MG_DEBUG_LOG // 디버그용
	public:
		void SetName(const std::string& _name)  { m_name = _name; }; 
		const std::string& GetName() const { return m_name; }
	private:
		std::string m_name;
#endif // MG_DEBUG

	protected:
		// 자식에서 세부 구현
		virtual bool Evaluate(const TravelResult& _result, const ProcessContext& _context) const = 0;

	private:
		bool m_bIsInvert{ false };		// 반전 여부
		bool m_bIsProcessed{ false };	// 플래그
		bool m_bProcessResult{ false }; // 처리 결과
	};

	class CompositeCondition : public ProcessCondition
	{
	public:
		void SetChildren(std::vector<std::weak_ptr<ProcessCondition>>&& _children);

	protected:
		std::vector<std::weak_ptr<ProcessCondition>> m_children;
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
		std::weak_ptr<ProcessCondition> m_pCondition;

#ifdef MG_DEBUG_LOG // 디버그용
	public:
		void SetName(const std::string& _name) { m_name = _name; };
		const std::string& GetName() const { return m_name; }
	private:
		std::string m_name;
#endif // MG_DEBUG
	};

#pragma endregion

	class ProcessorComponent : public Component
	{
	public:
		bool ProcessResult(const TravelResult& _inTravelResult, uint8_t& _outResult) const; // 탐색 결과를 주어진 조건 데이터에 맞게 처리
		void Init(std::vector<std::shared_ptr<ProcessCondition>>&& _conds, std::vector<std::shared_ptr<ProcessData>>&& _datas)
		{
			m_conditions = std::move(_conds);
			m_processDatas = std::move(_datas);
		};

	private:
		// 각 객체의 소유권은 컴포넌트에서 관리
		std::vector<std::shared_ptr<ProcessCondition>> m_conditions;
		std::vector<std::shared_ptr<ProcessData>> m_processDatas;
	};
}