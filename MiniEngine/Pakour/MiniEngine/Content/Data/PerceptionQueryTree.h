#pragma once

#include "Asset/DataAsset.h"
#include "Core/Math.h"

namespace MiniEngine
{
	class PerceptionNode;
	struct TravelContext;
}

/*
데이터 스키마
	"root" : "노드 객체 id 문자열",
	"decos" : [ ... 데코 객체 배열 ... ] // 노드에 붙여 쓰는 조건문 역할 객체
	"nodes" : [ ... 노드 객체 배열 ... ] // 인지 트리 구성 노드

데코 객체 구성
	{
		"comment" : "단순 주석 용도",

		"id" : "인스턴스 구분용 문자열",
		"class" : "클래스 이름",

		"isInverted" : true,

		// 각 노드별 멤버 변수
		"val_float" : 0.0,
		"val_uint8" : 0,
		"comparer" : "Greater", // "GEqual", "Equal", "LEqual", "Lesser"
        "targetObstacleType" : "Default", // 장애물 타입
		"targetState" : "Landing",
	}

노드 객체 구성
	{
		"comment" : "단순 주석 용도",

		"id" : "인스턴스 구분용 문자열",
		"class" : "노드가 쓰는 클래스 이름",
		"children" : [ "노드 Id 1", "노드 Id 2", ... ] // 합성 노드가 가지는 자식 노드들 배열
		"decos" : [ "데코 Id 1", "데코 Id 2", ... ] // 해당 노드에 대한 데코레이터 설정

		// 각 노드 개별 멤버 변수
		"startOffset" : [ 0.0, 0.0, 0.0 ],
		"direction" : [ 0.0, 0.0, 1.0 ],
		"distance" : 1.0,
		"radius" : 0.5,
		"height" : 1.0,

		"targetState" : "Landing"
	}
*/

struct PerceptionDecoData 
{
	std::string Id;
	std::string Class;

	bool IsInverted{ false };

	uint8_t ComparerType{ 0 }; // string name -> uint8_t 전환

	float ValueFloat{ 0.0f };
	uint8_t ValueUint8{ 0 };

	uint8_t TargetObstacleType{ 0U }; // -> ETagEnvDetail
	// Character::EState
	uint8_t TargetState{ 0 };
};

struct PerceptionNodeData
{
	std::string Id;
	std::string NodeClass;
	std::vector<std::string> Children;
	std::vector<std::string> Decos;

	// DetectNode
	MiniEngine::Vector3 StartOffset{ 0.0f, 0.0f, 0.0f };
	MiniEngine::Vector3 Direction{ 0.0f, 0.0f, 0.0f };
	float Distance{ 1.0f };
	float Radius{ 0.5f };
	float Height{ 1.0f };

	// Character::EState
	uint8_t TargetState{ 0 };
};

// 노드에 대한 json obj 참조가 발생
// 너무 깊은 곳으로 전파됨
// 각 노드들에게 읽기 양식 전달받기?
// 노드 객체에게 멤버변수 명칭을 받고
// 데이터는 멤버변수 명칭을 확인하고 있으면 Set

class PerceptionQueryData : public MiniEngine::DataAsset
{
public:
	void Load(const json& _data) override;
	std::shared_ptr<MiniEngine::PerceptionNode> ConstructTree();

	bool IsValid() const { return m_bValid; }
	const std::string& GetRootId() const { return m_rootId; }
	const std::vector<PerceptionNodeData>& GetNodes() const { return m_nodes; }

private:
	bool m_bValid{ false };
	std::string m_rootId;
	std::vector<PerceptionDecoData> m_decos;
	std::vector<PerceptionNodeData> m_nodes;
};
