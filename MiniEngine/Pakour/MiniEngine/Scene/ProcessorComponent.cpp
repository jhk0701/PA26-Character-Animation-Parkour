#include "pch.h"
#include "Scene/ProcessorComponent.h"
#include "Core/Log.h"

namespace MiniEngine 
{
#pragma region Process Nodes 인지 결과 처리용

	bool ProcessCondition::Process(const TravelResult& _result, const ProcessContext& _context)
	{
		bool bResult = Evaluate(_result, _context);

		if (m_bIsInvert)
			bResult = !bResult;

		return bResult;
	};

	void CompositeCondition::SetChildren(std::vector<std::weak_ptr<ProcessCondition>>&& _children)
	{
		m_children = std::move(_children);
	}

	bool ConditionAnd::Evaluate(const TravelResult& _result, const ProcessContext& _context) const
	{
		for (const std::weak_ptr<ProcessCondition>& pCond : m_children)
		{
#ifdef MG_DEBUG
			MG_LOG_INFO("[ConditionAnd] Process {}", pCond.lock()->GetName());
#endif // MG_DEBUG
			if (pCond.lock()->Process(_result, _context) == false)
				return false;
		}

		return true;
	}

	bool ConditionOr::Evaluate(const TravelResult& _result, const ProcessContext& _context) const
	{
		for (const std::weak_ptr<ProcessCondition>& pCond : m_children)
		{
#ifdef MG_DEBUG
			MG_LOG_INFO("[ConditionOr] Process {}", pCond.lock()->GetName());
#endif // MG_DEBUG
			if (pCond.lock()->Process(_result, _context) == true)
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
		if (m_pCondition.expired())
		{
			MG_LOG_INFO("[ProcessData::TryQuery] condition is not valid.");
			return false;
		}

		bool bResult = m_pCondition.lock()->Process(_inResult, _inContext);

		if (bResult)
			_outResult = m_result;

		return bResult;
	};

#pragma endregion

	bool ProcessorComponent::ProcessResult(const TravelResult& _inTravelResult, uint8_t& _outResult) const
	{
		ProcessContext context;
		context.pOwner = owner.lock();

#ifdef MG_DEBUG
		int i = 0;
#endif // MG_DEBUG

		for (const std::shared_ptr<ProcessData>& pProcess : m_processDatas)
		{
#ifdef MG_DEBUG
			MG_LOG_INFO("[ProcessorComponent::ProcessResult] Loop Process Data {}", i++);
#endif // DEBUG
			if (pProcess->TryQuery(_inTravelResult, context, _outResult))
				return true; // true 인 것이 있다면 바로 반환
		}

		return false;
	}


}