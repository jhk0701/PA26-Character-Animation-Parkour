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
	
	const Vector3 halfExtent = _desc.scale * 0.5f;
	std::shared_ptr<StaticMeshComponent> staticMeshComp = AddComponent<StaticMeshComponent>();
	staticMeshComp->SetColor(_desc.color);
	staticMeshComp->SetMesh(_desc.pMesh);
	staticMeshComp->localTransform.position = _desc.pos;
	staticMeshComp->localTransform.scale = halfExtent;
	staticMeshComp->localTransform.rotation = _desc.rot;

	std::shared_ptr<Physics::PhysicsWorld> phyWorld = GetScene()->GetPhysics().lock();
	std::shared_ptr<RigidBodyComponent> pRB = AddComponent<RigidBodyComponent>();
	pRB->Init(*phyWorld, RigidBodyComponent::EBodyType::Static, halfExtent, staticMeshComp);
	pRB->SetQueryLayer(_desc.layer);

	AddLedge(_desc);
}

void Obstacle::AddLedge(const ObstacleDesc& _desc)
{
	const Vector3 halfExtent = _desc.scale * 0.5f;
	const Vector3 commonLedgeExtentX = Vector3(halfExtent.x, 0.02f, 0.02f);
	const Vector3 commonLedgeExtentY = Vector3(halfExtent.z, 0.02f, 0.02f);

	switch (_desc.ledgeOpt)
	{
	case Single:
	{
		AddLedge(_desc.pos + Vector3(0.0f, halfExtent.y, 0.0f), 
			Vector3(halfExtent.x, 0.02f, halfExtent.z), 
			_desc.rot
		);
		break;
	}
	case Vertical:
	{
		m_pLedges.reserve(2);
		AddLedge(_desc.pos + Vector3(halfExtent.x, halfExtent.y, 0.0f),
			commonLedgeExtentY,
			Quaternion::CreateFromYawPitchRoll(ToRadians(90.0f), 0.0f, 0.0f) * _desc.rot
		);
		AddLedge(_desc.pos + Vector3(-halfExtent.x, halfExtent.y, 0.0f),
			commonLedgeExtentY,
			Quaternion::CreateFromYawPitchRoll(ToRadians(90.0f), 0.0f, 0.0f) * _desc.rot
		);
		break;
	}
	case Horizontal:
	{
		m_pLedges.reserve(2);
		AddLedge(_desc.pos + Vector3(0.0f, halfExtent.y, halfExtent.z),
			commonLedgeExtentX,
			Quaternion(0.0f, 0.0f, 0.0f, 1.0f) * _desc.rot
		);
		AddLedge(_desc.pos + Vector3(0.0f, halfExtent.y, -halfExtent.z),
			commonLedgeExtentX,
			Quaternion(0.0f, 0.0f, 0.0f, 1.0f) * _desc.rot
		);
		break;
	}
	case All:
	{
		m_pLedges.reserve(4);
		AddLedge(_desc.pos + Vector3(0.0f, halfExtent.y, halfExtent.z),
			commonLedgeExtentX,
			Quaternion(0.0f, 0.0f, 0.0f, 1.0f) * _desc.rot
		);
		AddLedge(_desc.pos + Vector3(0.0f, halfExtent.y, -halfExtent.z),
			commonLedgeExtentX,
			Quaternion(0.0f, 0.0f, 0.0f, 1.0f) * _desc.rot
		);
		AddLedge(_desc.pos + Vector3(halfExtent.x, halfExtent.y, 0.0f),
			commonLedgeExtentY,
			Quaternion::CreateFromYawPitchRoll(ToRadians(90.0f), 0.0f, 0.0f) * _desc.rot
		);
		AddLedge(_desc.pos + Vector3(-halfExtent.x, halfExtent.y, 0.0f),
			commonLedgeExtentY,
			Quaternion::CreateFromYawPitchRoll(ToRadians(90.0f), 0.0f, 0.0f) * _desc.rot
		);
		break;
	}
	case None: __fallthrough;
	default: return;
	}
}

void Obstacle::AddLedge(
	const Vector3& _localPos, 
	const Vector3& _halfExtent, 
	const Quaternion& _localRot)
{
	// Ledge 구성
	// box, static rigid body, size, position
	// scene, rigidbody comp
	std::shared_ptr<Physics::PhysicsWorld> phyWorld = GetScene()->GetPhysics().lock();
	std::shared_ptr<SceneComponent> pScene = AddComponent<SceneComponent>();
	pScene->localTransform.position = _localPos;
	pScene->localTransform.rotation = _localRot;
	pScene->localTransform.scale = _halfExtent;

	m_pLedges.push_back(pScene);

	std::shared_ptr<RigidBodyComponent> pRB = AddComponent<RigidBodyComponent>();
	pRB->Init(*phyWorld, RigidBodyComponent::EBodyType::Static, _halfExtent, pScene, 10.0f, true);
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
		float d = Vector3::DistanceSquared(pScene->localTransform.position, _pos);

		if (minDist > d)
		{
			result = pScene->localTransform.position.y;
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
