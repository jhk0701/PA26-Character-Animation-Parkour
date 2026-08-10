#include "pch.h"
#include <memory.h>
#include "Content/Obstacle.h"
#include "Content/ContentConfig.h"

#include "Scene/Scene.h"
#include "Scene/StaticMeshComponent.h"
#include "Scene/RigidBodyComponent.h"
#include "Scene/Tag.h"

using namespace MiniEngine;

void Obstacle::Construct(const ObstacleDesc& _desc)
{
	if (_desc.pMesh == nullptr)
		return;

	SetTickConfig(true, true, false);
	SetName("Obstacle");

	Tag& tag = GetTag();

	tag += (uint8_t)Content::Config::ETagEnv::Obstacle;
	tag += _desc.priority; // 우선 순위
	for (const uint8_t t : _desc.detailTags)
		tag += t;

	const Vector3 HALF_EXTENT = _desc.scale * 0.5f;
	
	std::shared_ptr<SceneComponent> root = AddComponent<SceneComponent>();
	root->localTransform.position = _desc.pos;
	root->localTransform.rotation = _desc.rot;

	std::shared_ptr<StaticMeshComponent> staticMeshComp = AddComponent<StaticMeshComponent>();
	staticMeshComp->AttachTo(root);
	staticMeshComp->SetColor(_desc.color);
	staticMeshComp->SetMesh(_desc.pMesh);
	staticMeshComp->localTransform.scale = HALF_EXTENT;
	staticMeshComp->localTransform.position = _desc.meshPos;

	std::shared_ptr<Physics::PhysicsWorld> phyWorld = GetScene()->GetPhysics().lock();
	std::shared_ptr<RigidBodyComponent> pRB = AddComponent<RigidBodyComponent>();
	pRB->Init(*phyWorld, RigidBodyComponent::EBodyType::Static, HALF_EXTENT, staticMeshComp, root);
	pRB->SetQueryLayer(_desc.layer);

	AddLedge(root,
		_desc.meshPos + Vector3(0.0f, HALF_EXTENT.y, 0.0f),
		Vector3(HALF_EXTENT.x, 0.01f, HALF_EXTENT.z));
}

void Obstacle::AddLedge(
	std::shared_ptr<SceneComponent> _parent,
	const Vector3& _localPos, 
	const Vector3& _halfExtent)
{
	if (!_parent)
		return;

	std::shared_ptr<Physics::PhysicsWorld> phyWorld = GetScene()->GetPhysics().lock();
		std::shared_ptr<SceneComponent> pScene = AddComponent<SceneComponent>();
	pScene->AttachTo(_parent);
	pScene->localTransform.position = _localPos;

	m_pLedges.push_back(pScene);

	std::shared_ptr<RigidBodyComponent> pRB = AddComponent<RigidBodyComponent>();
	pRB->Init(*phyWorld, RigidBodyComponent::EBodyType::Static, _halfExtent, pScene, _parent, 10.0f, true);
	pRB->SetQueryLayer(MiniEngine::Physics::Layer::ObstacleLedge);
}

float Obstacle::GetNearestLedgeHeight(const Vector3& _pos) const
{
	// 소형 장애물(Beam, Protrude)에서 주로 사용할 것
	float minDist = FLT_MAX;

	float result = GetRoot()->localTransform.position.y;
	for (int i = 0; i < m_pLedges.size(); ++i)
	{
		std::shared_ptr<SceneComponent> pScene = m_pLedges[i].lock();
		
		Transform worldTF;
		pScene->GetWorldTransform(worldTF);
		Vector3 worldPos = worldTF.position;

		float d = Vector3::DistanceSquared(worldPos, _pos);

		if (minDist > d)
		{
			result = worldPos.y;
			minDist = d;
		}
	}

	return result;
}

bool Obstacle::TryGetTag(uint8_t _idx, uint8_t& _outTag)
{
	return GetTag().GetTagAt(_idx, _outTag);
}

const Transform& Obstacle::GetTransform() const
{
	return GetRoot()->localTransform;
}

#ifdef MG_DEBUG_LOG
const std::string& Obstacle::DebugName()
{
	return GetName();
}
#endif

std::shared_ptr<Actor> ObstacleFactory::Create(
	std::shared_ptr<Scene> _pScene, const Obstacle::ObstacleDesc& _desc)
{
	if (_pScene == nullptr)
		return nullptr;

	std::shared_ptr<Obstacle> pObs = _pScene->SpawnActor<Obstacle>();
	pObs->Construct(_desc);

	return pObs;
}
