#include "pch.h"
#include "Scene/ProcessorComponent.h"

namespace MiniEngine 
{
#pragma region Process Nodes 인지 결과 처리용

	void CompositeCondition::SetChildren(std::vector<std::shared_ptr<ProcessCondition>>&& _children)
	{
		m_children = std::move(_children);
	}

	bool ConditionAnd::Evaluate(const TravelResult& _result, const ProcessContext& _context) const
	{
		for (const std::shared_ptr<ProcessCondition>& pCond : m_children)
		{
			if (pCond->Evaluate(_result, _context) == false)
				return false;
		}

		return true;
	}

	bool ConditionOr::Evaluate(const TravelResult& _result, const ProcessContext& _context) const
	{
		for (const std::shared_ptr<ProcessCondition>& pCond : m_children)
		{
			if (pCond->Evaluate(_result, _context) == true)
				return true;
		}

		return false;
	}

	void ProcessData::Init(uint8_t _result, std::shared_ptr<ProcessCondition> _pCondition)
	{
		m_result = _result;
		m_pCondition = _pCondition;
	}

	bool ProcessData::TryQuery(const TravelResult& _inResult, const ProcessContext& _inContext, uint8_t& _outResult) const
	{
		if (!m_pCondition)
			return false;

		bool bResult = m_pCondition->Evaluate(_inResult, _inContext);

		if (bResult)
			_outResult = m_result;

		return bResult;
	};

#pragma endregion

	bool ProcessorComponent::ProcessResult(const TravelResult& _inTravelResult, uint8_t& _outResult) const
	{
		ProcessContext context;
		context.pOwner = owner.lock();

		for (const std::shared_ptr<ProcessData>& pProcess : m_processDatas)
		{
			if (pProcess->TryQuery(_inTravelResult, context, _outResult))
				return true; // true 인 것이 있다면 바로 반환
		}

		return false;
	}

}