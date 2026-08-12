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

	// Character::EState
	uint8_t TargetState{ 0 };
};

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
	std::vector<PerceptionNodeData> m_nodes;
};
