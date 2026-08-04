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
			{ 
				"ObstacleTypeCondition", 
				{ 
					[](const ConditionSchema& _data) 
					{
						std::shared_ptr<ObstacleTypeCondition> pCond = std::make_shared<ObstacleTypeCondition>();
						pCond->SetType(_data.TargetType);
						return pCond;
					}
				}
			},
			{ 
				"ObstacleHeightCondition", 
				{ 
					[](const ConditionSchema& _data) 
					{
						std::shared_ptr<ObstacleHeightCondition> pCond = std::make_shared<ObstacleHeightCondition>();
						pCond->SetValue(_data.Value);
						return pCond;
					}
				}
			},
			{ 
				"ObstacleDepthCondition", 
				{ 
					[](const ConditionSchema& _data) 
					{
						std::shared_ptr<ObstacleDepthCondition> pCond = std::make_shared<ObstacleDepthCondition>();
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

			auto itChildren = el.find("children");
			if (itChildren != _data.end() && itChildren->is_array()) 
			{
				cond.Children.reserve(itChildren->size());
				for (const json& child : *itChildren)
					cond.Children.push_back(child.get<std::string>());
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
	// 임시 검증
	// 조건 재사용
	std::shared_ptr<ObstacleTypeCondition> pIsDefault = std::make_shared<ObstacleTypeCondition>();
	pIsDefault->SetType((uint8_t)ETagEnvDetail::Default);

	std::shared_ptr<ObstacleHeightCondition> pHeightCheckWall = std::make_shared<ObstacleHeightCondition>();
	pHeightCheckWall->SetValue(3.0f);

	std::shared_ptr<ObstacleHeightCondition> pHeightCheckHigh = std::make_shared<ObstacleHeightCondition>();
	pHeightCheckHigh->SetValue(2.0f);

	std::shared_ptr<ObstacleHeightCondition> pHeightCheckLow = std::make_shared<ObstacleHeightCondition>();
	pHeightCheckLow->SetValue(1.0f);

	std::shared_ptr<ObstacleDepthCondition> pIsDeeper = std::make_shared<ObstacleDepthCondition>();
	pIsDeeper->SetValue(1.0f);

	std::shared_ptr<ObstacleDepthCondition> pIsNotDeeper = std::make_shared<ObstacleDepthCondition>();
	pIsNotDeeper->SetValue(1.0f);
	pIsNotDeeper->Invert(true);

	// Landing Wall Hanging
	std::shared_ptr<ConditionAnd> pLandingWallHang = std::make_shared<ConditionAnd>();
	pLandingWallHang->SetChildren({ pIsDefault, pHeightCheckWall });

	std::shared_ptr<ProcessData> pLandingWallHangData = std::make_shared<ProcessData>();
	pLandingWallHangData->Init((uint8_t)ETagAct::Wall_IdleToHang, pLandingWallHang);

	// Landing Vault High
	std::shared_ptr<ConditionAnd> pLandingVaultHigh = std::make_shared<ConditionAnd>();
	pLandingVaultHigh->SetChildren({ pIsDefault, pHeightCheckHigh, pIsNotDeeper });

	std::shared_ptr<ProcessData> pLandingVaultHighData = std::make_shared<ProcessData>();
	pLandingVaultHighData->Init((uint8_t)ETagAct::VaultHigh, pLandingVaultHigh);

	// Landing Vault Low, Mid
	std::shared_ptr<ConditionAnd> pLandingVaultLowMid = std::make_shared<ConditionAnd>();
	pLandingVaultLowMid->SetChildren({ pIsDefault, pHeightCheckLow, pIsNotDeeper });

	std::shared_ptr<ProcessData> pLandingVaultMidData = std::make_shared<ProcessData>();
	pLandingVaultMidData->Init((uint8_t)ETagAct::VaultMid, pLandingVaultLowMid);
	std::shared_ptr<ProcessData> pLandingVaultLowData = std::make_shared<ProcessData>();
	pLandingVaultLowData->Init((uint8_t)ETagAct::VaultLow, pLandingVaultLowMid);

	// Landing Mantle High
	std::shared_ptr<ConditionAnd> pLandingMantleHigh = std::make_shared<ConditionAnd>();
	pLandingMantleHigh->SetChildren({ pIsDefault, pHeightCheckHigh, pIsDeeper });

	std::shared_ptr<ProcessData> pLandingMantleHighData = std::make_shared<ProcessData>();
	pLandingMantleHighData->Init((uint8_t)ETagAct::MantleHigh, pLandingMantleHigh);

	// Landing Mantle Low, Mid
	std::shared_ptr<ConditionAnd> pLandingMantleLowMid = std::make_shared<ConditionAnd>();
	pLandingMantleLowMid->SetChildren({ pIsDefault, pHeightCheckLow, pIsDeeper });

	std::shared_ptr<ProcessData> pLandingMantleMidData = std::make_shared<ProcessData>();
	pLandingMantleMidData->Init((uint8_t)ETagAct::MantleMid, pLandingMantleLowMid);
	std::shared_ptr<ProcessData> pLandingMantleLowData = std::make_shared<ProcessData>();
	pLandingMantleLowData->Init((uint8_t)ETagAct::MantleLow, pLandingMantleLowMid);

	_out = 
	{
		pLandingWallHangData,
		
		pLandingVaultHighData,
		pLandingVaultMidData,
		pLandingVaultLowData,

		pLandingMantleHighData,
		pLandingMantleMidData,
		pLandingMantleLowData
	};
}