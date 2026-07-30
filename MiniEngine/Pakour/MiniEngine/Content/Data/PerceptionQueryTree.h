#pragma once

#include "Asset/DataAsset.h"
#include "Core/Math.h"

namespace MiniEngine
{
	class PerceptionNode;
	struct TravelContext;
}

struct PerceptionNodeData
{
	std::string Id;
	std::string NodeClass;
	std::vector<std::string> Children;

	// CheckObstacleNode
	float HeightMultiplier{ 1.0f };
	MiniEngine::Vector3 StartOffset{ 0.0f, 0.0f, 0.0f };

	// TransitionCharacterFSMNode
	uint8_t TargetState{ 0 };
};

struct PerceptionConfig
{
	// 장애물 탐지 거리
	float minObstacleDetectDist = 1.0f;
	float maxObstacleDetectDist = 2.5f;

	// 장애물 파쿠르 기준
	float thresholdWallHeight = 3.0f;
	float thresholdHighObstacle = 2.0f;
	float thresholdLowObstacle = 1.0f;

	// 일반 장애물 측량 파라미터
	float heightRadius = 0.5f;	// heightStep과 함께 바꿀 것
	float heightStep = 1.0f;	// = heightRadius * 2 여야 밴드가 틈이 없음
	uint8_t maxHeightStep = 3;	// 3.0m 이상은 벽으로 보고 매달린다
	float heightSearchtDist = 0.1f;

	float depthStep = 0.5f; // 확인하는 depth 깊이 단위 0.5m
	uint8_t maxDepthStep = 2;	// 최대 1.0m 까지만 잰다
	float depthSearchDownDist = 2.0f;
	float depthLift = 0.05f; // 꼭대기 표면인 경우 방지를 위해 띄어두는 크기
	float minMantleDepth = 1.0f;	// 이 값 이상이어야 Mantle, 미만은 Vault

	// 매달린 상태(Hanging)의 주변 탐색 파라미터
	float onHangingSearchDist = 1.5f;	// 상하좌우 공통 탐지 거리
	float onHangingSearchRadius = 0.5f;	// 상/좌/우 스피어 프로브 반지름 (하방은 단순 레이)
};


class PerceptionQueryData : public MiniEngine::DataAsset
{
public:
	void Load(const json& _data) override;

	bool IsValid() const { return m_bValid; }
	const std::string& GetRootId() const { return m_rootId; }
	const std::vector<PerceptionNodeData>& GetNodes() const { return m_nodes; }

private:
	bool m_bValid{ false };
	std::string m_rootId;
	std::vector<PerceptionNodeData> m_nodes;
};

// 쿼리 트리 팩토리용도 -> 콘텐츠 코드에서 사용할 것
class PerceptionQueryTree
{
public:
	// 실패 시 nullptr 반환(사유는 에러 로그).
	std::shared_ptr<MiniEngine::PerceptionNode> ConstructTree(const PerceptionQueryData& _data);
};
