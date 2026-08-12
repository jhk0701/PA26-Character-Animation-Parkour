#include "pch.h"
#include "Content/Data/PerceptionQueryTree.h"

#include "Perception/Config/ObstacleConfig.h"
#include "Perception/PerceptionComponent.h"

#include "Content/ContentConfig.h"
#include "Content/Character.h"

#include "Perception/Node/ReturnNode.h"
#include "Perception/Node/DetectObstacleNode.h"
#include "Perception/Node/MeasureObstacleNode.h"
#include "Perception/Node/ProcessObstacleNode.h"
#include "Perception/Node/Decorator/InheritDecorator.h"
#include "Perception/Node/Decorator/ObstacleDecorator.h"

#include "Content/Perception/SelectCharacterStateNode.h"
#include "Content/Perception/SelectObstacleTagNode.h"
#include "Content/Perception/SelectUsingHeightNode.h"
#include "Content/Perception/SelectUsingInputDirNode.h"
#include "Content/Perception/CheckObstacleNode.h"
#include "Content/Perception/MeasureNode.h"
#include "Content/Perception/BeamCompareHeightNode.h"
#include "Content/Perception/ProtrudeExtractHeightNode.h"
#include "Content/Perception/CheckObstacleOnHangingNode.h"

#include "Content/Perception/Decorator/CharacterDecorator.h"
#include "Content/Perception/DetectObstacleUsingInputNode.h"

#include <functional>
#include <unordered_map>

#include "Core/Log.h"

using namespace MiniEngine;

namespace
{
#pragma region Node Registry

	using FuncCreateDeco = std::function<std::shared_ptr<PerceptionDecorator>(const PerceptionDecoData&)>;
	using FuncCreateNode = std::function<std::shared_ptr<PerceptionNode>(const PerceptionNodeData&)>;

	constexpr int CHILDREN_ANY = -1;
	constexpr int CONDITION_CHILDREN = 2;

	struct DecoSpec 
	{
		FuncCreateDeco Create;
	};

	struct NodeSpec
	{
		FuncCreateNode Create;
		int ExpectedChildren{ 0 };
	};

	template<typename T>
	FuncCreateDeco CreateDeco() 
	{
		return [](const PerceptionDecoData&) -> std::shared_ptr<PerceptionDecorator>
		{
			return std::make_shared<T>();
		};
	}

	template<typename T>
	FuncCreateNode CreateNode()
	{
		return [](const PerceptionNodeData&) -> std::shared_ptr<PerceptionNode>
		{
			return std::make_shared<T>();
		};
	}

	template<typename T>
	FuncCreateNode CreateDetectNode()
	{
		return [](const PerceptionNodeData& _data) -> std::shared_ptr<PerceptionNode>
		{
			std::shared_ptr<T> p = std::make_shared<T>();
			p->SetStartOffset(_data.StartOffset);
			p->SetDirection(_data.Direction);
			p->SetDistance(_data.Distance);
			p->SetRadius(_data.Radius);
			return p;
		};
	}

	void DecoRegistry(std::unordered_map<std::string, DecoSpec>& _out) 
	{
		_out =
		{
			{ "ObstacleDetectedDecorator",		{ CreateDeco<ObstacleDetectedDecorator>() } },
			{ "CompareObstacleTypeDecorator",	
				{ 
					[](const PerceptionDecoData& _data) ->std::shared_ptr<PerceptionDecorator>
					{
						std::shared_ptr<CompareObstacleTypeDecorator> pDeco = std::make_shared<CompareObstacleTypeDecorator>();
						pDeco->SetComparer((ECompareType)_data.ComparerType);
						pDeco->SetValue(_data.TargetObstacleType);
						return pDeco;
					}
				}
			},
			{ "CompareHeightDecorator", 
				{
					[](const PerceptionDecoData& _data) ->std::shared_ptr<PerceptionDecorator>
					{
						std::shared_ptr<CompareHeightDecorator> pDeco = std::make_shared<CompareHeightDecorator>();
						pDeco->SetComparer((ECompareType)_data.ComparerType);
						pDeco->SetValue(_data.ValueFloat);
						return pDeco;
					}
				}
			},

			// 콘텐츠 추가
			{ "CharacterStateDecorator", 
				{
					[](const PerceptionDecoData& _data) ->std::shared_ptr<PerceptionDecorator>
					{
						std::shared_ptr<CharacterStateDecorator> pDeco = std::make_shared<CharacterStateDecorator>();
						pDeco->SetComparer((ECompareType)_data.ComparerType);
						pDeco->SetValue(_data.TargetState);
						return pDeco;
					}
				}
			},
			{ "InputVerticalDecorator",
				{
					[](const PerceptionDecoData& _data) ->std::shared_ptr<PerceptionDecorator>
					{
						std::shared_ptr<InputVerticalDecorator> pDeco = std::make_shared<InputVerticalDecorator>();
						pDeco->SetComparer((ECompareType)_data.ComparerType);
						pDeco->SetValue(_data.ValueFloat);
						return pDeco;
					}
				}
			},
			{ "InputHorizontalDecorator",
				{
					[](const PerceptionDecoData& _data) ->std::shared_ptr<PerceptionDecorator>
					{
						std::shared_ptr<InputHorizontalDecorator> pDeco = std::make_shared<InputHorizontalDecorator>();
						pDeco->SetComparer((ECompareType)_data.ComparerType);
						pDeco->SetValue(_data.ValueFloat);
						return pDeco;
					}
				}
			}
		};
	}

	// 새 인식 노드 클래스는 아래에 추가
	void NodeRegistry(std::unordered_map<std::string, NodeSpec>& _out)
	{
		_out =
		{
			{ "SequenceNode",				{ CreateNode<SequenceNode>(),				CHILDREN_ANY } },
			{ "SelectorNode",				{ CreateNode<SelectorNode>(),				CHILDREN_ANY } },

			// Task (Leaf)
			{ "ReturnResultNode",			{ CreateNode<ReturnResultNode>(),			0 } },
			{ "ReturnEmptyNode",			{ CreateNode<ReturnEmptyNode>(),			0 } },
			{ "DetectObstacleCapsuleNode",	
				{
					[](const PerceptionNodeData& _node) -> std::shared_ptr<PerceptionNode>
					{
						std::shared_ptr<DetectObstacleCapsuleNode> p = std::make_shared<DetectObstacleCapsuleNode>();
						p->SetStartOffset(_node.StartOffset);
						p->SetDirection(_node.Direction);
						p->SetDistance(_node.Distance);
						p->SetRadius(_node.Radius);
						p->SetCapsuleHeight(_node.Height);
						p->SetHeightMultiplier(_node.HeightMultiplier);
						return p;
					}, 0
				}
			},
			{ "DetectObstacleSphereNode",	{ CreateDetectNode<DetectObstacleSphereNode>(), 0} },
			{ "DetectLedgeNode",			{ CreateDetectNode<DetectLedgeNode>(),			0} },
			{ "DetectLedgeMultipleNode",	{ CreateDetectNode<DetectLedgeMultipleNode>(),	0} },
			{ "DetectFloorNode",			{ CreateDetectNode<DetectFloorNode>(),			0} },
			{ "CheckObstacleSphereNode",	{ CreateDetectNode<CheckObstacleSphereNode>(),	0} },
			{ "MeasureObstacleHeightNode",	
				{ 
					[](const PerceptionNodeData& _node) -> std::shared_ptr<PerceptionNode>
					{
						std::shared_ptr<MeasureObstacleHeightNode> p = std::make_shared<MeasureObstacleHeightNode>();
						p->SetDirection(_node.Direction);
						return p;
					}, 0
				} 
			},
			{ "MeasureObstacleDepthNode",	
				{
					[](const PerceptionNodeData& _node) -> std::shared_ptr<PerceptionNode>
					{
						std::shared_ptr<MeasureObstacleDepthNode> p = std::make_shared<MeasureObstacleDepthNode>();
						p->SetDirection(_node.Direction);
						return p;
					}, 0
				} 
			},
			{ "ProcessBeamNode",			{ CreateNode<ProcessBeamNode>(),			0 } },
			{ "ProcessProtrudeNode",		{ CreateNode<ProcessProtrudeNode>(),		0 } },
			{ "ProcessPoleNode",			
				{
					[](const PerceptionNodeData& _node) -> std::shared_ptr<PerceptionNode>
					{
						std::shared_ptr<ProcessPoleNode> p = std::make_shared<ProcessPoleNode>();
						p->SetHeight(_node.Height);
						return p;
					}, 0 
				} 	
			},

			// 콘텐츠
			{ "DetectObstacleUsingInputNode",	{ CreateDetectNode<DetectObstacleUsingInputNode>(), 0} },

			// 구버전 Task
			{ "MeasureDepthNode",			{ CreateNode<MeasureDepthNode>(),			0 } },
			{ "MeasureHeightNode",			{ CreateNode<MeasureHeightNode>(),			0 } },
			{ "MeasureDepth_SideNode",		{ CreateNode<MeasureDepth_SideNode>(),		0 } },
			{ "MeasureHeight_SideNode",		{ CreateNode<MeasureHeight_SideNode>(),		0 } },
			{ "BeamCompareHeightNode",		{ CreateNode<BeamCompareHeightNode>(),		0 } },
			{ "ProtrudeExtractHeightNode",	{ CreateNode<ProtrudeExtractHeightNode>(),	0 } }, // 

			// Switch : 자식 중 Success 나올 때까지 연속 호출
			{ "SelectCharacterStateNode",	{ CreateNode<SelectCharacterStateNode>(),	(int)Character::EState::End } },
			{ "SelectObstacleTagNode",		{ CreateNode<SelectObstacleTagNode>(),		(int)ETagEnvDetail::End } },
			{ "SelectUsingHeightNode",		{ CreateNode<SelectUsingHeightNode>(),		3 } },	// 무시 / 깊이측정 / 벽
			{ "SelectUsingInputDirNode",	{ CreateNode<SelectUsingInputDirNode>(),	4 } },	// 상 / 하 / 좌우 / 입력없음 // 
			{ "SelectUsingInputVerticalNode",	{ CreateNode<SelectUsingInputVerticalNode>(),	3 } },

			// Condition — children[0] = true, children[1] = false
			{ "CheckOnHangingMoveUpNode",	{ CreateNode<CheckOnHangingMoveUpNode>(),	CONDITION_CHILDREN } },
			{ "CheckOnHangingMoveDownNode",	{ CreateNode<CheckOnHangingMoveDownNode>(),	CONDITION_CHILDREN } },

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
				} 
			},
			{ "CheckObstacleTowardInputDirNode", 
				{
					[](const PerceptionNodeData& _node) -> std::shared_ptr<PerceptionNode>
					{
						std::shared_ptr<CheckObstacleTowardInputDirNode> pNode = std::make_shared<CheckObstacleTowardInputDirNode>();
						pNode->SetStartOffset(_node.StartOffset);
						return pNode;
					},
					CONDITION_CHILDREN
				}
			},
			{ "CheckOnHangingUpwardLedgeNode",	
				{ 
					[](const PerceptionNodeData& _node) -> std::shared_ptr<PerceptionNode>
					{
						std::shared_ptr<CheckOnHangingUpwardLedgeNode> pNode = std::make_shared<CheckOnHangingUpwardLedgeNode>();
						pNode->SetStartOffset(_node.StartOffset);
						return pNode;
					}, 
					CONDITION_CHILDREN 
				} 
			},
			{ "CheckOnHangingMoveSideNode",
				{
					[](const PerceptionNodeData& _node) -> std::shared_ptr<PerceptionNode>
					{
						std::shared_ptr<CheckOnHangingMoveSideNode> pNode = std::make_shared<CheckOnHangingMoveSideNode>();
						pNode->SetStartOffset(_node.StartOffset);
						return pNode;
					},
					CONDITION_CHILDREN
				}
			},
			{ "CheckOnHangingMoveToInputDirNode",
				{
					[](const PerceptionNodeData& _node) -> std::shared_ptr<PerceptionNode>
					{
						std::shared_ptr<CheckOnHangingMoveToInputDirNode> pNode = std::make_shared<CheckOnHangingMoveToInputDirNode>();
						pNode->SetStartOffset(_node.StartOffset);
						return pNode;
					},
					CONDITION_CHILDREN
				}
			},
			{ "PoleExtractDataNode",
				{
					[](const PerceptionNodeData& _node) -> std::shared_ptr<PerceptionNode>
					{
						std::shared_ptr<PoleExtractDataNode> pNode = std::make_shared<PoleExtractDataNode>();
						pNode->SetHeightLimit(_node.HeightMultiplier);
						return pNode;
					},
					0 
				} 
			},
		};
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
	m_decos.clear();
	m_nodes.clear();

	try
	{
		m_rootId = _data.value("root", std::string());
		if (m_rootId.empty())
		{
			MG_LOG_ERROR("[PerceptionQueryData] 'root' is missing.");
			return;
		}

		// deco 읽기
		auto itDecos = _data.find("decos");
		if (itDecos == _data.end() || itDecos->is_array() == false) 
		{
			MG_LOG_ERROR("[PerceptionQueryData] 'decos' array is missing.");
			return;
		}

		m_decos.reserve(itDecos->size());

		for (const json& el : *itDecos)
		{
			PerceptionDecoData deco;
			deco.Id = el.value("id", std::string());
			deco.Class = el.value("class", std::string());

			if (deco.Id.empty() || deco.Class.empty())
			{
				MG_LOG_ERROR("[PerceptionQueryData] decorator needs both 'id' and 'class'.");
				return;
			}

			// 멤버 변수 파싱
			deco.ValueFloat = el.value("val_float", deco.ValueFloat);
			deco.ValueUint8 = el.value("val_uint8", deco.ValueUint8);
			const std::string COMPARER_NAME = el.value("comparer", std::string());
			if (COMPARER_NAME.empty() == false) 
			{
				if (TryParseComparerType(COMPARER_NAME, deco.ComparerType) == false) 
				{
					MG_LOG_ERROR("[PerceptionQueryData] deco '{}' has unknown Comparer '{}'.", deco.Id, COMPARER_NAME);
					return;
				}
			}

			const std::string OBS_TYPE_NAME = el.value("targetObstacleType", std::string());
			if (OBS_TYPE_NAME.empty() == false)
			{
				if (TryParseTagEnvDetail(OBS_TYPE_NAME, deco.TargetObstacleType) == false)
				{
					MG_LOG_ERROR("[PerceptionQueryData] deco '{}' has unknown targetObstacleType '{}'.", deco.Id, OBS_TYPE_NAME);
					return;
				}
			}

			const std::string STATE_NAME = el.value("targetState", std::string());
			if (STATE_NAME.empty() == false)
			{
				if (Character::TryParseState(STATE_NAME, deco.TargetState) == false)
				{
					MG_LOG_ERROR("[PerceptionQueryData] deco '{}' has unknown targetState '{}'.",
						deco.Id, STATE_NAME);
					return;
				}
			}

			m_decos.push_back(std::move(deco));
		}

		// node 읽기
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

			auto itDecos = element.find("decos");
			if (itDecos != element.end() && itDecos->is_array()) 
			{
				node.Decos.reserve(itDecos->size());
				for (const json& deco : *itDecos)
					node.Decos.push_back(deco.get<std::string>());
			}

			// TODO : 매개변수들은 따로 처리할 방법이 필요
			// 현재는 우선 node에 멤버변수로 계속 포함시킴
			ReadVec3(element, "startOffset", node.StartOffset);
			ReadVec3(element, "direction", node.Direction);
			node.Distance			= element.value("distance", node.Distance);
			node.Radius				= element.value("radius", node.Radius);
			node.Height				= element.value("height", node.Height);
			node.HeightMultiplier	= element.value("heightMultiplier", node.HeightMultiplier);

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
		m_decos.clear();
		return;
	}

	m_bValid = true;
}

// 콘텐츠에서 사용할 지형 인식 로직
std::shared_ptr<PerceptionNode> PerceptionQueryData::ConstructTree()
{
	if (IsValid() == false)
	{
		MG_LOG_ERROR("[PerceptionQueryTree] query data is invalid.");
		return nullptr;
	}

	std::unordered_map<std::string, DecoSpec> decoRegistry;
	DecoRegistry(decoRegistry);
	std::unordered_map<std::string, NodeSpec> nodeRegistry;
	NodeRegistry(nodeRegistry);

	std::unordered_map<std::string, std::shared_ptr<PerceptionDecorator>> createdDeco;
	createdDeco.reserve(m_decos.size());

	// 데이터 읽고 노드 생성
	std::unordered_map<std::string, std::shared_ptr<PerceptionNode>> createdNode;
	createdNode.reserve(m_nodes.size());

	for (const PerceptionDecoData& DECO : m_decos)
	{
		auto itSpec = decoRegistry.find(DECO.Class);
		if (itSpec == decoRegistry.end()) 
		{
			MG_LOG_ERROR("[PerceptionQueryTree] unknown deco class '{}' (id '{}').",
				DECO.Class, DECO.Id);
			return nullptr;
		}

		if (createdDeco.find(DECO.Id) != createdDeco.end())
		{
			MG_LOG_ERROR("[PerceptionQueryTree] duplicated deco id '{}'.", DECO.Id);
			return nullptr;
		}

		createdDeco[DECO.Id] = itSpec->second.Create(DECO);
	}

	for (const PerceptionNodeData& NODE : m_nodes)
	{
		auto itSpec = nodeRegistry.find(NODE.NodeClass);
		if (itSpec == nodeRegistry.end())
		{
			MG_LOG_ERROR("[PerceptionQueryTree] unknown node class '{}' (id '{}').",
				NODE.NodeClass, NODE.Id);
			return nullptr;
		}

		if (createdNode.find(NODE.Id) != createdNode.end())
		{
			MG_LOG_ERROR("[PerceptionQueryTree] duplicated node id '{}'.", NODE.Id);
			return nullptr;
		}

		createdNode[NODE.Id] = itSpec->second.Create(NODE);
	}

	// 자식 및 데코레이터 설정
	for (const PerceptionNodeData& NODE : m_nodes)
	{
		const std::shared_ptr<PerceptionNode>& SELF = createdNode[NODE.Id];

		// Deco 붙이기
		std::vector<std::shared_ptr<PerceptionDecorator>> decos;
		decos.reserve(NODE.Decos.size());

		for (const std::string& DECO_ID : NODE.Decos)
		{
			auto it = createdDeco.find(DECO_ID);
			if (it == createdDeco.end())
			{
				MG_LOG_ERROR("[PerceptionQueryTree] deco '{}' references unknown id '{}'.", NODE.Id, DECO_ID);
				return nullptr;
			}

			std::shared_ptr<PerceptionDecorator> pDeco = it->second;
			if (pDeco == nullptr)
				return nullptr;

			decos.push_back(std::move(pDeco));
		}
		SELF->SetConditions(std::move(decos));

		const int EXPECTED = nodeRegistry.at(NODE.NodeClass).ExpectedChildren;
		const bool bIsComposite = (std::dynamic_pointer_cast<CompositeNode>(SELF) != nullptr);
		if (bIsComposite == false)
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
			auto it = createdNode.find(CHILD_ID);
			if (it == createdNode.end())
			{
				MG_LOG_ERROR("[PerceptionQueryTree] node '{}' references unknown id '{}'.", NODE.Id, CHILD_ID);
				return nullptr;
			}

			std::shared_ptr<PerceptionNode> pChild = it->second;
			if (pChild == nullptr)
				return nullptr;

			children.push_back(std::move(pChild));
		}

		if (bIsComposite)
			std::static_pointer_cast<CompositeNode>(SELF)->SetChildren(std::move(children));
	}

	auto itRoot = createdNode.find(GetRootId());
	if (itRoot == createdNode.end())
	{
		MG_LOG_ERROR("[PerceptionQueryTree] root id '{}' not found.", GetRootId());
		return nullptr;
	}

	MG_LOG_INFO("[PerceptionQueryTree] constructed {} nodes. root = '{}'.",
		createdNode.size(), GetRootId());

	return itRoot->second;
}
