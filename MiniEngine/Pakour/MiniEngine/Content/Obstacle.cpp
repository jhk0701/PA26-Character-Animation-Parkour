#include "pch.h"
#include <memory.h>
#include "Content/Obstacle.h"
#include "Content/ContentConfig.h"

#include "Scene/Scene.h"
#include "Scene/StaticMeshComponent.h"
#include "Scene/RigidBodyComponent.h"
#include "Scene/Tag.h"

using namespace MiniEngine;

void Obstacle::Construct(std::shared_ptr<StaticMesh> _pStaticMesh, 
	const Vector3& _pos, 
	const Vector3& _scale,
	const Quaternion& _rot,
	const std::vector<uint8_t>& _detailTags,
	ELedgeOption _opt)
{
	if (_pStaticMesh == nullptr)
		return;
	
	SetName("Obstacle");

	Tag& tag = GetTag();
	tag += (uint8_t)Content::Config::ETagEnv::Obstacle;
	for (const uint8_t t : _detailTags)
		tag += t;
	
	const Vector3 halfExtent = _scale * 0.5f;
	std::shared_ptr<StaticMeshComponent> staticMeshComp = AddComponent<StaticMeshComponent>();
	staticMeshComp->SetColor(Vector3(0.7f, 0.5f, 0.2f));
	staticMeshComp->SetMesh(_pStaticMesh);
	staticMeshComp->localTransform.position = _pos;
	staticMeshComp->localTransform.scale = halfExtent;
	staticMeshComp->localTransform.rotation = _rot;


	std::shared_ptr<Physics::PhysicsWorld> phyWorld = GetScene()->GetPhysics().lock();
	std::shared_ptr<RigidBodyComponent> pRB = AddComponent<RigidBodyComponent>();
	pRB->Init(*phyWorld, RigidBodyComponent::EBodyType::Static, halfExtent, staticMeshComp);
	pRB->SetQueryLayer(MiniEngine::Physics::Layer::Obstacle);

	if (_opt == ELedgeOption::None)
		return;

	std::shared_ptr<Actor> pSharedThis = shared_from_this();
	const Vector3 commonLedgeExtentX = Vector3(halfExtent.x, 0.02f, 0.02f);
	const Vector3 commonLedgeExtentY = Vector3(halfExtent.z, 0.02f, 0.02f);

	m_pLedges.reserve(_opt == ELedgeOption::All ? 4 : 2);
	
	if (_opt == ELedgeOption::All || _opt == ELedgeOption::Horizontal)
	{
		AddLedge(pSharedThis,
			_pos + Vector3(0.0f, halfExtent.y, halfExtent.z),
			commonLedgeExtentX,
			Quaternion(0.0f, 0.0f, 0.0f, 1.0f) * _rot
		);
		AddLedge(pSharedThis,
			_pos + Vector3(0.0f, halfExtent.y, -halfExtent.z),
			commonLedgeExtentX,
			Quaternion(0.0f, 0.0f, 0.0f, 1.0f) * _rot
		);
	}

	if (_opt == ELedgeOption::All || _opt == ELedgeOption::Vertical) 
	{
		AddLedge(pSharedThis,
			_pos + Vector3(halfExtent.x, halfExtent.y, 0.0f),
			commonLedgeExtentY,
			Quaternion::CreateFromYawPitchRoll(ToRadians(90.0f), 0.0f, 0.0f) * _rot
		);
		AddLedge(pSharedThis,
			_pos + Vector3(-halfExtent.x, halfExtent.y, 0.0f),
			commonLedgeExtentY,
			Quaternion::CreateFromYawPitchRoll(ToRadians(90.0f), 0.0f, 0.0f) * _rot
		);
	}
}

void Obstacle::AddLedge(std::shared_ptr<Actor> _pTarget, 
	const Vector3& _localPos, 
	const Vector3& _halfExtent, 
	const Quaternion& _localRot)
{
	// Ledge 구성
	// box, static rigid body, size, position
	// scene, rigidbody comp
	std::shared_ptr<Physics::PhysicsWorld> phyWorld = GetScene()->GetPhysics().lock();
	std::shared_ptr<SceneComponent> pScene = _pTarget->AddComponent<SceneComponent>();
	pScene->localTransform.position = _localPos;
	pScene->localTransform.rotation = _localRot;
	pScene->localTransform.scale = _halfExtent;

	m_pLedges.push_back(pScene);

	std::shared_ptr<RigidBodyComponent> pRB = _pTarget->AddComponent<RigidBodyComponent>();
	pRB->Init(*phyWorld, RigidBodyComponent::EBodyType::Static, _halfExtent, pScene, 10.0f, true);
	pRB->SetQueryLayer(MiniEngine::Physics::Layer::ObstacleLedge);
}

float Obstacle::GetNearestLedgeHeight(const Vector3& _pos) const
{
	float result = FLT_MAX;
	for (std::weak_ptr<SceneComponent> pScene : m_pLedges)
		result = min(result, Vector3::DistanceSquared(pScene.lock()->localTransform.position, _pos));

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

std::shared_ptr<Actor> ObstacleFactory::Create(
	std::shared_ptr<Scene> _pScene, 
	std::shared_ptr<StaticMesh> _pStaticMesh,
	const Vector3& _pos, 
	const Vector3& _scale,
	const Quaternion& _rot,
	const std::vector<uint8_t>& _detailTags,
	Obstacle::ELedgeOption _opt)
{
	if (_pScene == nullptr)
		return nullptr;

	std::shared_ptr<Obstacle> pObs = _pScene->SpawnActor<Obstacle>();
	pObs->Construct(_pStaticMesh, _pos, _scale, _rot, _detailTags, _opt);

	return pObs;
}
