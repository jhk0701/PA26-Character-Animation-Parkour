#pragma once
#include "Asset/DataAsset.h"

namespace MiniEngine 
{
    class ProcessCondition;
	class ProcessData;
}

/*
데이터 스키마
{
    "condition":
    {
        "comment" : "조건들은 재사용 가능, 매개변수 가능",
        "id": "인스턴스의 고유 명칭, 숫자로 했다가 휴먼에러 가능성이 높아져 문자열로 변경",
        "class": "사용할 클래스",
        "children" : "Composite Condition 유형들은 자식 포함 [ 자식1_ID, 자식2_ID, ... ]",
        "isInverted" : true,
        "value" : 0,
        "targetType" : "Default"
    },
    "conditions" : [],

    "processData":
    {
        "comment" : "특정 액터 행동과 실행할 조건 1:1 대응",
        "tagAct" : "VaultMid" // "조건이 만족하면 실행할 액션 (ETagAct)",
        "conditionId" : "사용할 조건문"
    },
    "processDatas" : []
}
*/

struct ConditionSchema
{
    std::string Id;
    std::string CondClass;

    std::vector<std::string> Children;

    // 사용 매개변수
    // 공통 사항
    bool IsInverted{ false };
    // ObstacleHeightCondition, ObstacleDepthCondition
    float Value{ 0.0f };
    // ObstacleTypeCondition
    uint8_t TargetType{ 0U }; // -> ETagEnvDetail

};

struct ProcessDataSchema
{
    uint8_t TagAct; // -> ETagAct 
    std::string ConditionId; // 사용할 조건문
};

class ProcessConditionData : public MiniEngine::DataAsset
{
public:
	void Load(const json& _data) override;
	void ConstructData(
        std::vector<std::shared_ptr<MiniEngine::ProcessCondition>>& _outConditions, 
        std::vector<std::shared_ptr<MiniEngine::ProcessData>>& _outProcessData);

    bool IsValid() const { return m_bIsValid; }

private:
    bool m_bIsValid{ false };
    std::unordered_map<std::string, ConditionSchema> m_condDatas;
    std::vector<ProcessDataSchema> m_processDatas;

};
