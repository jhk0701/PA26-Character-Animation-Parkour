#include "pch.h"
#include "Content/Data/PerceptionQueryTree.h"
#include "Scene/PerceptionComponent.h"
#include "Core/Log.h"

#include "Content/ContentConfig.h"
#include "Content/Character.h"

#include "Content/Perception/ReturnNode.h"
#include "Content/Perception/SelectCharacterStateNode.h"
#include "Content/Perception/SelectObstacleTagNode.h"
#include "Content/Perception/SelectUsingHeightNode.h"
#include "Content/Perception/SelectUsingInputDirNode.h"
#include "Content/Perception/CheckObstacleNode.h"
#include "Content/Perception/MeasureDepthNode.h"
#include "Content/Perception/BeamCompareHeightNode.h"
#include "Content/Perception/ProtrudeExtractHeightNode.h"
#include "Content/Perception/CheckObstacleOnHangingNode.h"
#include "Content/Perception/TransitionCharacterFSMNode.h"

#include <functional>
#include <unordered_map>
#include <unordered_set>

using namespace MiniEngine;

namespace
{
#pragma region Node Registry

	using NodeCreator = std::function<std::shared_ptr<PerceptionNode>(const PerceptionNodeData&)>;

	constexpr int CHILDREN_ANY = -1;
	constexpr int CONDITION_CHILDREN = 2;

	struct NodeSpec
	{
		NodeCreator Create;
		int ExpectedChildren{ 0 };
	};

	template<typename T>
	NodeCreator CreateNode()
	{
		return [](const PerceptionNodeData&) -> std::shared_ptr<PerceptionNode>
			{
				return std::make_shared<T>();
			};
	}

	// 새 인식 노드 클래스를 추가하면 여기에 등록해야 json 에서 쓸 수 있다.
	const std::unordered_map<std::string, NodeSpec>& NodeRegistry()
	{
		static const std::unordered_map<std::string, NodeSpec> REGISTRY =
		{
			// 합성 노드
			{ "SequenceNode",				{ CreateNode<SequenceNode>(),				CHILDREN_ANY } },

			// Task (Leaf)
			{ "ReturnResultNode",			{ CreateNode<ReturnResultNode>(),			0 } },
			{ "ReturnEmptyNode",			{ CreateNode<ReturnEmptyNode>(),			0 } },
			{ "MeasureDepthNode",			{ CreateNode<MeasureDepthNode>(),			0 } },
			{ "BeamCompareHeightNode",		{ CreateNode<BeamCompareHeightNode>(),		0 } },
			{ "ProtrudeExtractHeightNode",	{ CreateNode<ProtrudeExtractHeightNode>(),	0 } },

			// Selector — 자식 개수가 InvokeCondition 반환 범위와 같아야 한다
			{ "SelectCharacterStateNode",	{ CreateNode<SelectCharacterStateNode>(),	(int)Character::EState::End } },
			{ "SelectObstacleTagNode",		{ CreateNode<SelectObstacleTagNode>(),		(int)Content::Config::ETagEnvDetail::End } },
			{ "SelectUsingHeightNode",		{ CreateNode<SelectUsingHeightNode>(),		3 } },	// 무시 / 깊이측정 / 벽
			{ "SelectUsingInputDirNode",	{ CreateNode<SelectUsingInputDirNode>(),	4 } },	// 상 / 하 / 좌우 / 입력없음

			// Condition — children[0] = true, children[1] = false
			{ "CheckOnHangingMoveUpNode",	{ CreateNode<CheckOnHangingMoveUpNode>(),	CONDITION_CHILDREN } },
			{ "CheckOnHangingMoveDownNode",	{ CreateNode<CheckOnHangingMoveDownNode>(),	CONDITION_CHILDREN } },
			{ "CheckOnHangingMoveSideNode",	{ CreateNode<CheckOnHangingMoveSideNode>(),	CONDITION_CHILDREN } },

			// 파라미터를 갖는 노드
			{ "CheckObstacleNode",
				{
					[](const PerceptionNodeData& _node) -> std::shared_ptr<PerceptionNode>
					{
						std::shared_ptr<CheckObstacleNode> pNode = std::make_shared<CheckObstacleNode>();
						pNode->SetHeightMultiplier(_node.HeightMultiplier);
						pNode->SetStartOffset(_node.StartOffset);
						return pNode;
					},
					CONDITION_CHILDREN
				} },
			{ "TransitionCharacterFSMNode",
				{
					[](const PerceptionNodeData& _node) -> std::shared_ptr<PerceptionNode>
					{
						std::shared_ptr<TransitionCharacterFSMNode> pNode = std::make_shared<TransitionCharacterFSMNode>();
						pNode->SetTargetState(_node.TargetState);
						return pNode;
					},
					0
				} },
		};

		return REGISTRY;
	}

#pragma endregion

	void ReadVec3(const json& _data, const char* _key, Vector3& _out)
	{
		auto it = _data.find(_key);
		if (it == _data.end() || it->is_array() == false || it->size() != 3)
			return;

		_out = Vector3((*it)[0].get<float>(), (*it)[1].get<float>(), (*it)[2].get<float>());
	}
}

void PerceptionQueryData::Load(const json& _data)
{
	m_bValid = false;
	m_rootId.clear();
	m_nodes.clear();

	try
	{
		m_rootId = _data.value("root", std::string());
		if (m_rootId.empty())
		{
			MG_LOG_ERROR("[PerceptionQueryData] 'root' is missing.");
			return;
		}

		auto itNodes = _data.find("nodes");
		if (itNodes == _data.end() || itNodes->is_array() == false)
		{
			MG_LOG_ERROR("[PerceptionQueryData] 'nodes' array is missing.");
			return;
		}

		m_nodes.reserve(itNodes->size());

		for (const json& element : *itNodes)
		{
			PerceptionNodeData node;
			node.Id = element.value("id", std::string());
			node.NodeClass = element.value("class", std::string());

			if (node.Id.empty() || node.NodeClass.empty())
			{
				MG_LOG_ERROR("[PerceptionQueryData] node needs both 'id' and 'class'.");
				return;
			}

			auto itChildren = element.find("children");
			if (itChildren != element.end() && itChildren->is_array())
			{
				node.Children.reserve(itChildren->size());
				for (const json& child : *itChildren)
					node.Children.push_back(child.get<std::string>());
			}

			// TODO : 매개변수들은 따로 처리할 방법이 필요
			// 현재는 우선 node에 멤버변수로 계속 포함시킴
			node.HeightMultiplier = element.value("heightMultiplier", node.HeightMultiplier);
			ReadVec3(element, "startOffset", node.StartOffset);

			const std::string STATE_NAME = element.value("targetState", std::string());
			if (STATE_NAME.empty() == false)
			{
				if (Character::TryParseState(STATE_NAME, node.TargetState) == false)
				{
					MG_LOG_ERROR("[PerceptionQueryData] node '{}' has unknown targetState '{}'.",
						node.Id, STATE_NAME);
					return;
				}
			}

			m_nodes.push_back(std::move(node));
		}
	}
	catch (const json::exception& e)
	{
		MG_LOG_ERROR("[PerceptionQueryData] parse failed : {}", e.what());
		m_nodes.clear();
		return;
	}

	m_bValid = true;
}

// 콘텐츠에서 사용할 지형 인식 로직
std::shared_ptr<PerceptionNode> PerceptionQueryTree::ConstructTree(const PerceptionQueryData& _data)
{
	if (_data.IsValid() == false)
	{
		MG_LOG_ERROR("[PerceptionQueryTree] query data is invalid.");
		return nullptr;
	}

	const std::vector<PerceptionNodeData>& NODES = _data.GetNodes();
	const std::unordered_map<std::string, NodeSpec>& REGISTRY = NodeRegistry();

	// 데이터 읽고 노드 생성
	std::unordered_map<std::string, std::shared_ptr<PerceptionNode>> created;
	created.reserve(NODES.size());

	for (const PerceptionNodeData& NODE : NODES)
	{
		auto itSpec = REGISTRY.find(NODE.NodeClass);
		if (itSpec == REGISTRY.end())
		{
			MG_LOG_ERROR("[PerceptionQueryTree] unknown node class '{}' (id '{}').",
				NODE.NodeClass, NODE.Id);
			return nullptr;
		}

		if (created.find(NODE.Id) != created.end())
		{
			MG_LOG_ERROR("[PerceptionQueryTree] duplicated node id '{}'.", NODE.Id);
			return nullptr;
		}

		created[NODE.Id] = itSpec->second.Create(NODE);
	}

	// 혹시 없는 노드가 데이터에 있는 경우 로깅
	auto resolve = [&created](const std::string& _id, const std::string& _ownerId) -> std::shared_ptr<PerceptionNode>
		{
			auto it = created.find(_id);
			if (it == created.end())
			{
				MG_LOG_ERROR("[PerceptionQueryTree] node '{}' references unknown id '{}'.", _ownerId, _id);
				return nullptr;
			}

			return it->second;
		};

	// 자식 설정
	for (const PerceptionNodeData& NODE : NODES)
	{
		const std::shared_ptr<PerceptionNode>& SELF = created[NODE.Id];
		const int EXPECTED = REGISTRY.at(NODE.NodeClass).ExpectedChildren;

		const bool bIsSelector = (std::dynamic_pointer_cast<SelectorNode>(SELF) != nullptr);
		const bool bIsCondition = (std::dynamic_pointer_cast<ConditionNode>(SELF) != nullptr);
		const bool bIsSequence = (std::dynamic_pointer_cast<SequenceNode>(SELF) != nullptr);

		if (bIsSelector == false && bIsCondition == false && bIsSequence == false)
		{
			// TaskNode — 자식을 가질 수 없다
			if (NODE.Children.empty() == false)
			{
				MG_LOG_ERROR("[PerceptionQueryTree] node '{}' ({}) can't have children.",
					NODE.Id, NODE.NodeClass);
				return nullptr;
			}

			continue;
		}

		if (EXPECTED != CHILDREN_ANY && (int)NODE.Children.size() != EXPECTED)
		{
			MG_LOG_ERROR("[PerceptionQueryTree] node '{}' ({}) needs {} children but has {}.",
				NODE.Id, NODE.NodeClass, EXPECTED, NODE.Children.size());
			return nullptr;
		}

		if (NODE.Children.empty())
		{
			MG_LOG_ERROR("[PerceptionQueryTree] node '{}' ({}) has no children.",
				NODE.Id, NODE.NodeClass);
			return nullptr;
		}

		std::vector<std::shared_ptr<PerceptionNode>> children;
		children.reserve(NODE.Children.size());

		for (const std::string& CHILD_ID : NODE.Children)
		{
			std::shared_ptr<PerceptionNode> pChild = resolve(CHILD_ID, NODE.Id);
			if (pChild == nullptr)
				return nullptr;

			children.push_back(std::move(pChild));
		}

		if (bIsSelector)
			std::static_pointer_cast<SelectorNode>(SELF)->SetChildren(std::move(children));
		else if (bIsCondition)
			std::static_pointer_cast<ConditionNode>(SELF)->SetChildren(std::move(children));
		else
			std::static_pointer_cast<SequenceNode>(SELF)->SetChildren(std::move(children));
	}

	auto itRoot = created.find(_data.GetRootId());
	if (itRoot == created.end())
	{
		MG_LOG_ERROR("[PerceptionQueryTree] root id '{}' not found.", _data.GetRootId());
		return nullptr;
	}

	MG_LOG_INFO("[PerceptionQueryTree] constructed {} nodes. root = '{}'.",
		created.size(), _data.GetRootId());

	return itRoot->second;
}
