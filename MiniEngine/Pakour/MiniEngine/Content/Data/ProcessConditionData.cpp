#include "pch.h"
#include "Content/Data/ProcessConditionData.h"
#include "Scene/ProcessorComponent.h"
#include "Content/ContentConfig.h"
#include "Content/Processor/ObstacleCondition.h"
#include "Core/Log.h"

#include <functional>

using namespace MiniEngine;
using namespace Content::Config;

namespace 
{
	using FuncCreateCond = std::function<std::shared_ptr<ProcessCondition>(const ConditionSchema&)>;
	
	struct ConditionSpec
	{
		FuncCreateCond CreateFunc;
	};

	template<typename T>
	FuncCreateCond CreateCond()
	{
		return [](const ConditionSchema& _data) -> std::shared_ptr<ProcessCondition>
		{
			std::shared_ptr<T> pCond = std::make_shared<T>();
			pCond->Invert(_data.IsInverted);
			return pCond;
		};
	}

	// 클래스 등록
	const std::unordered_map<std::string, ConditionSpec>& ConditionRegistry()
	{
		// 1번만 생성하도록 static으로 선언
		static const std::unordered_map<std::string, ConditionSpec> CONDITION_REGISTRY =
		{
			{ "ConditionAnd",	{ CreateCond<ConditionAnd>() } },
			{ "ConditionOr",	{ CreateCond<ConditionOr>() } },
			{ "ObstacleTypeCondition", 
				{ 
					[](const ConditionSchema& _data) 
					{
						std::shared_ptr<ObstacleTypeCondition> pCond = std::make_shared<ObstacleTypeCondition>();
						pCond->Invert(_data.IsInverted);
						pCond->SetType(_data.TargetType);
						return pCond;
					}
				}
			},
			{ "ObstacleHeightCondition", 
				{ 
					[](const ConditionSchema& _data) 
					{
						std::shared_ptr<ObstacleHeightCondition> pCond = std::make_shared<ObstacleHeightCondition>();
						pCond->Invert(_data.IsInverted);
						pCond->SetValue(_data.Value);
						return pCond;
					}
				}
			},
			{ "ObstacleDepthCondition", 
				{ 
					[](const ConditionSchema& _data) 
					{
						std::shared_ptr<ObstacleDepthCondition> pCond = std::make_shared<ObstacleDepthCondition>();
						pCond->Invert(_data.IsInverted);
						pCond->SetValue(_data.Value);
						return pCond;
					}
				}
			},
		};

		return CONDITION_REGISTRY;
	}

}

void ProcessConditionData::Load(const json& _data)
{
	// json 데이터 파싱
	m_bIsValid = false;
	m_condDatas.clear();
	m_processDatas.clear();

	try 
	{
		auto itCond = _data.find("conditions");
		if (itCond == _data.end() || itCond->is_array() == false)
		{
			MG_LOG_ERROR("[ProcessConditionData] 'conditions' is missing.");
			return;
		}

		m_condDatas.reserve(itCond->size());
		for (const json& el : *itCond)
		{
			ConditionSchema cond;
			cond.Id = el.value("id", std::string());
			cond.CondClass = el.value("class", std::string());

			if (cond.Id.empty() && cond.CondClass.empty()) 
			{
				MG_LOG_ERROR("[ProcessConditionData] 'Id', 'class' is missing.");
				return;
			}

			if (el.contains("children")) 
			{
				auto itChildren = el.find("children");
				if (itChildren != el.end() && itChildren->is_array())
				{
					cond.Children.reserve(itChildren->size());
					for (const json& child : *itChildren)
						cond.Children.push_back(child.get<std::string>());
				}
			}

			cond.IsInverted = el.value("isInverted", cond.IsInverted);
			cond.Value = el.value("value", cond.Value);
			
			const std::string TYPE_NAME = el.value("targetType", std::string());
			if (TYPE_NAME.empty() == false)
			{
				if (TryParseTagEnvDetail(TYPE_NAME, cond.TargetType) == false)
				{
					MG_LOG_ERROR("[ProcessConditionData] condition '{}' has unknown targetState '{}'.",
						cond.Id, TYPE_NAME);
					return;
				}
			}

			m_condDatas.insert({ cond.Id, std::move(cond) });
		}
	}
	catch (const json::exception& e)
	{
		MG_LOG_ERROR("[ProcessConditionData] 'conditions' parse failed : {}", e.what());
		m_condDatas.clear();
		return;
	}

	try 
	{
		auto itDatas = _data.find("processDatas");
		if (itDatas == _data.end() || itDatas->is_array() == false)
		{
			MG_LOG_ERROR("[ProcessConditionData] 'processDatas' is missing.");
			return;
		}

		m_processDatas.reserve(itDatas->size());

		for (const json& el : *itDatas)
		{
			ProcessDataSchema data;

			const std::string TAG_ACT_NAME = el.value("tagAct", std::string());
			if (TAG_ACT_NAME.empty() == false) 
			{
				if (TryParseTagAct(TAG_ACT_NAME, data.TagAct) == false)
				{
					MG_LOG_ERROR("[ProcessConditionData] 'tagAct' is missing.");
					return;
				}
			}

			data.ConditionId = el.value("conditionId", std::string());
			if (data.ConditionId.empty()) 
			{
				MG_LOG_ERROR("[ProcessConditionData] 'conditionId' is missing.");
				return;
			}

			m_processDatas.push_back(std::move(data));
		}
	}
	catch (const json::exception& e)
	{
		MG_LOG_ERROR("[ProcessConditionData] 'processDatas' parse failed : {}", e.what());
		m_processDatas.clear();
		return;
	}

	m_bIsValid = true;
}

void ProcessConditionData::ConstructData(std::vector<std::shared_ptr<MiniEngine::ProcessData>>& _out)
{
	if (IsValid() == false)
	{
		MG_LOG_ERROR("[ProcessConditionData::ConstructData] Data is no valid. check data files.");
		return;
	}

	_out.clear();
	_out.reserve(m_processDatas.size());
	const std::unordered_map<std::string, ConditionSpec>& REGISTRY = ConditionRegistry(); // Class - CreateFunc

	// condition 생성
	std::unordered_map<std::string, std::shared_ptr<ProcessCondition>> mapCondition; // Id - Instance
	mapCondition.reserve(m_condDatas.size());
	for (const std::pair<std::string, const ConditionSchema>& pair : m_condDatas)
	{
		auto it = REGISTRY.find(pair.second.CondClass);
		if (it == REGISTRY.end())
		{
			MG_LOG_ERROR("[ProcessConditionData::ConstructData] {} class is not registered.", pair.first);
			return;
		}

		mapCondition.insert( { pair.first, it->second.CreateFunc(pair.second) } );
	}

	// 자식 할당
	for (const std::pair<std::string, const ConditionSchema>& pair : m_condDatas)
	{
		if (pair.second.Children.empty())
			continue;

		std::shared_ptr<CompositeCondition> pComp = std::dynamic_pointer_cast<CompositeCondition>(mapCondition[pair.first]);
		if (!pComp)
			continue;
		
		std::vector<std::shared_ptr<ProcessCondition>> children;
		children.reserve(pair.second.Children.size());
		for (const std::string& child : pair.second.Children)
			children.push_back(mapCondition[child]);

		pComp->SetChildren(std::move(children));
	}

	// process 생성 후 Condtion 할당
	for (const ProcessDataSchema& p : m_processDatas)
	{
		std::shared_ptr<ProcessData> pProcessData = std::make_shared<ProcessData>();
		pProcessData->Init(p.TagAct, mapCondition[p.ConditionId]);

		_out.push_back(pProcessData);
	}
}