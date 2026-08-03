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
