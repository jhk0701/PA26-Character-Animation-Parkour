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
	bool _addLedge)
{
	if (_pStaticMesh == nullptr)
		return;
	
	SetName("Obstacle");

	Tag& tag = GetTag();
	tag += (uint8_t)Content::Config::ETagEnv::Obstacle;

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

	const Vector3 commonLedgeExtentX = Vector3(halfExtent.x, 0.1f, 0.1f);
	const Vector3 commonLedgeExtentY = Vector3(halfExtent.z, 0.1f, 0.1f);

	if (_addLedge == false)
		return;

	std::shared_ptr<Actor> pSharedThis = shared_from_this();
	AddLedge(pSharedThis,
		_pStaticMesh,
		_pos + Vector3(0.0f, halfExtent.y, halfExtent.z),
		commonLedgeExtentX,
		Quaternion(0.0f, 0.0f, 0.0f, 1.0f) * _rot
	);
	AddLedge(pSharedThis,
		_pStaticMesh,
		_pos + Vector3(0.0f, halfExtent.y, -halfExtent.z),
		commonLedgeExtentX,
		Quaternion(0.0f, 0.0f, 0.0f, 1.0f) * _rot
	);
	AddLedge(pSharedThis,
		_pStaticMesh,
		_pos + Vector3(halfExtent.x, halfExtent.y, 0.0f),
		commonLedgeExtentY,
		Quaternion::CreateFromYawPitchRoll(ToRadians(90.0f), 0.0f, 0.0f) * _rot
	);
	AddLedge(pSharedThis,
		_pStaticMesh,
		_pos + Vector3(-halfExtent.x, halfExtent.y, 0.0f),
		commonLedgeExtentY,
		Quaternion::CreateFromYawPitchRoll(ToRadians(90.0f), 0.0f, 0.0f) * _rot
	);

	
}

void Obstacle::AddLedge(std::shared_ptr<Actor> _pTarget, 
	std::shared_ptr<StaticMesh> _pStaticMesh, 
	const Vector3& _localPos, 
	const Vector3& _halfExtent, 
	const Quaternion& _localRot)
{
	// Ledge 구성
	// box, static rigid body, size, position
	// scene, rigidbody comp
	std::shared_ptr<Physics::PhysicsWorld> phyWorld = GetScene()->GetPhysics().lock();
	std::shared_ptr<StaticMeshComponent> pSubMesh = _pTarget->AddComponent<StaticMeshComponent>();
	pSubMesh->SetMesh(_pStaticMesh);
	pSubMesh->SetColor(Vector3(1.0f));
	pSubMesh->localTransform.position = _localPos;
	pSubMesh->localTransform.rotation = _localRot;
	pSubMesh->localTransform.scale = _halfExtent;

	std::shared_ptr<RigidBodyComponent> pRB = _pTarget->AddComponent<RigidBodyComponent>();
	pRB->Init(*phyWorld, RigidBodyComponent::EBodyType::Static, _halfExtent, pSubMesh, 10.0f, true);
	pRB->SetQueryLayer(MiniEngine::Physics::Layer::ObstacleLedge);
}

std::shared_ptr<Actor> ObstacleFactory::Create(
	std::shared_ptr<Scene> _pScene, 
	std::shared_ptr<StaticMesh> _pStaticMesh,
	const Vector3& _pos, 
	const Vector3& _scale,
	const Quaternion& _rot,
	bool _addLedge)
{
	if (_pScene == nullptr)
		return nullptr;

	std::shared_ptr<Obstacle> pObs = _pScene->SpawnActor<Obstacle>();
	pObs->Construct(_pStaticMesh, _pos, _scale, _rot, _addLedge);

	return pObs;
}
