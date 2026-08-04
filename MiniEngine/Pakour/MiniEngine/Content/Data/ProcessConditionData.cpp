#include "pch.h"
#include "Content/Data/ProcessConditionData.h"
#include "Scene/ProcessorComponent.h"

#include "Content/ContentConfig.h"
#include "Content/Processor/ObstacleCondition.h"

using namespace MiniEngine;
using namespace Content::Config;

void ProcessConditionData::Load(const json& _data)
{
	// TODO : json 데이터 파싱

}

void ProcessConditionData::ConstructData(std::vector<std::shared_ptr<MiniEngine::ProcessData>>& _out)
{
	// 임시 검증
	// Char FSM의 Default 부분 구현
	/*_out = {
		std::make_shared<ProcessData>()->Init(
			(uint8_t)ETagAct::VaultHigh,
			std::make_shared<ConditionAnd>()->SetChildren(
				{
					std::make_shared<ObstacleTypeCondition>()
				})
		)
	};*/

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