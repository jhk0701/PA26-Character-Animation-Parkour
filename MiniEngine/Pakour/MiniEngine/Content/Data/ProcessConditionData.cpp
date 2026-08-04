#include "pch.h"
#include "Content/Data/ProcessConditionData.h"
#include "Scene/ProcessorComponent.h"
#include "Content/ContentConfig.h"

using namespace Content::Config;

void ProcessConditionData::Load(const json& _data)
{
	// TODO : json 데이터 파싱

}

void ProcessConditionData::ConstructData(std::vector<std::shared_ptr<MiniEngine::ProcessData>>& _out)
{
	// 임시 검증
	// Char FSM의 Default 부분 구현

	// ETagAct::Vault
}