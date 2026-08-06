#include "pch.h"
#include "Scene/ProcessorComponent.h"
#include "Core/Log.h"

namespace MiniEngine 
{
#pragma region Process Nodes 인지 결과 처리용

	bool ProcessCondition::Process(const TravelResult& _result, const ProcessContext& _context)
	{
		// 비용 절약용 처리
		if (m_bIsProcessed)
			return m_bProcessResult;

		bool bResult = Evaluate(_result, _context);

		if (m_bIsInvert)
			bResult = !bResult;

		m_bIsProcessed = true;
		m_bProcessResult = bResult;

		return m_bProcessResult;
	};

	void CompositeCondition::SetChildren(std::vector<std::weak_ptr<ProcessCondition>>&& _children)
	{
		m_children = std::move(_children);
	}

	bool ConditionAnd::Evaluate(const TravelResult& _result, const ProcessContext& _context) const
	{
		for (const std::weak_ptr<ProcessCondition>& pCond : m_children)
		{
			bool bResult = pCond.lock()->Process(_result, _context);

#ifdef MG_DEBUG
			MG_LOG_INFO("[ConditionAnd] Process {} :: {}", pCond.lock()->GetName(), bResult ? "true" : "false");
#endif // MG_DEBUG

			if (bResult == false)
				return false;
		}

		return true;
	}

	bool ConditionOr::Evaluate(const TravelResult& _result, const ProcessContext& _context) const
	{
		for (const std::weak_ptr<ProcessCondition>& pCond : m_children)
		{
			bool bResult = pCond.lock()->Process(_result, _context);

#ifdef MG_DEBUG
			MG_LOG_INFO("[ConditionOr] Process {} :: {}", pCond.lock()->GetName(), bResult ? "true" : "false");
#endif // MG_DEBUG

			if (bResult == true)
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
		for (const std::shared_ptr<ProcessCondition>& pCond : m_conditions)
			pCond->Reset();

		ProcessContext context;
		context.pOwner = owner.lock();

		MG_LOG_INFO("[ProcessorComponent::ProcessResult] Start Process Result ========================== ");
		for (const std::shared_ptr<ProcessData>& pProcess : m_processDatas)
		{
#ifdef MG_DEBUG
			MG_LOG_INFO("[ProcessorComponent::ProcessResult] Loop :: TryQuery Process Data : {}", pProcess->GetName());
#endif // DEBUG
			if (pProcess->TryQuery(_inTravelResult, context, _outResult))
				return true; // true 인 것이 있다면 바로 반환
		}

		return false;
	}


}