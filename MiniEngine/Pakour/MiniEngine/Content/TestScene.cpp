#include "pch.h"
#include "TestScene.h"
#include "Content/ContentConfig.h"
#include "Manager/PathManager.h"
#include "Manager/AssetManager.h"
#include "Scene/StaticMeshComponent.h"
#include "Scene/RigidBodyComponent.h"
#include "Scene/Tag.h"

#include "Content/Character.h"
#include "Core/Log.h"


using namespace MiniEngine;

TestScene::TestScene()
{
}
TestScene::~TestScene()
{
}

void TestScene::Construct(ID3D11Device* _device, ID3D11DeviceContext* _context)
{
	Scene::Construct(_device, _context);

	std::shared_ptr<Physics::PhysicsWorld> physics = GetPhysics().lock();

	std::shared_ptr<StaticMesh> pCubeMesh;
	std::wstring assetPath = PathManager::GetInstance()->ResolveAssetPath(L"Cube.mini");
	pCubeMesh = AssetManager::GetInstance()->LoadStaticMesh(assetPath);

	{
		// 강체 바닥 설치
		const Vector3 size(100.0f, 0.5f, 100.0f);
		std::shared_ptr<Actor> pGround = SpawnActor<Actor>();
		pGround->SetName("Ground");

		Tag& tag = pGround->GetTag();
		tag += (uint8_t)Content::Config::ETagEnv::Land;
		tag += (uint8_t)Content::Config::ETagAct::Landing;

		std::shared_ptr<StaticMeshComponent> pMeshComp = pGround->AddComponent<StaticMeshComponent>();
		pMeshComp->SetMesh(pCubeMesh);
		pMeshComp->localTransform.position = Vector3(0.0f, -0.5f, 0.0f);
		pMeshComp->localTransform.scale = size;

		std::shared_ptr<RigidBodyComponent> pRB = pGround->AddComponent<RigidBodyComponent>();
		pRB->Init(*physics, RigidBodyComponent::EBodyType::Static, size, pMeshComp);
		pRB->SetQueryLayer(MiniEngine::Physics::Layer::Ground);
	}
	{
		BuildObstacle(pCubeMesh, Vector3(-5.0f, 1.0f, 3.0f), Vector3(2.0f));

		BuildObstacle(pCubeMesh, Vector3(0.0f, 0.5f, 3.0f), Vector3(4.0f, 1.0f, 0.5f));
		
		BuildObstacle(pCubeMesh, Vector3(5.0f, 0.5f, 5.0f), Vector3(4.0f, 1.0f, 5.0f));
		BuildObstacle(pCubeMesh, Vector3(5.0f, 1.0, 7.5f),	Vector3(4.0f, 2.0f, 0.5f));

		BuildObstacle(pCubeMesh, Vector3(10.0f, 0.5f, 5.0f), Vector3(4.0f, 1.0f, 5.0f));
		BuildObstacle(pCubeMesh, Vector3(10.0f, 1.5f, 6.0f), Vector3(4.0f, 4.0f, 3.0f));
		BuildObstacle(pCubeMesh, Vector3(10.0f, 3.0f, 7.5f), Vector3(4.0f, 1.0f, 0.5f));

		BuildObstacle(pCubeMesh, Vector3(15.0f, 0.5f, 5.0f), Vector3(4.0f, 1.0f, 5.0f));
		BuildObstacle(pCubeMesh, Vector3(15.0f, 3.5f, 6.0f), Vector3(4.0f, 7.0f, 3.0f));
	}
	{
		// 임시 캐릭터 생성
		std::shared_ptr<Character> pChar = SpawnActor<Character>();
		pChar->SetName("Character");
		pChar->Construct();
	}
}

void TestScene::BeginPlay()
{
	Scene::BeginPlay();

#ifdef MG_DEBUG
	ApplyPhysicsDebug(true);
#endif // MG_DEBUG
}

std::shared_ptr<Actor> TestScene::BuildObstacle(std::shared_ptr<StaticMesh> _pStaticMesh, const Vector3& _pos, const Vector3& _scale)
{
	if (_pStaticMesh == nullptr)
		return nullptr;

	std::shared_ptr<Actor> ObstacleActor;
	ObstacleActor = SpawnActor<Actor>();
	ObstacleActor->SetName("Obstacle");
	
	Tag& tag = ObstacleActor->GetTag();
	tag += (uint8_t)Content::Config::ETagEnv::Obstacle;

	const Vector3 halfExtent = _scale * 0.5f;
	std::shared_ptr<StaticMeshComponent> staticMeshComp = ObstacleActor->AddComponent<StaticMeshComponent>();
	staticMeshComp->SetColor(Vector3(0.7f, 0.5f, 0.2f));
	staticMeshComp->SetMesh(_pStaticMesh);
	staticMeshComp->localTransform.position = _pos;
	staticMeshComp->localTransform.scale = halfExtent;

	std::shared_ptr<Physics::PhysicsWorld> phyWorld = GetPhysics().lock();

	std::shared_ptr<RigidBodyComponent> pRB = ObstacleActor->AddComponent<RigidBodyComponent>();
	pRB->Init(*phyWorld, RigidBodyComponent::EBodyType::Static, halfExtent, staticMeshComp);
	pRB->SetQueryLayer(MiniEngine::Physics::Layer::Obstacle);

	const Vector3 commonLedgeExtentX = Vector3(halfExtent.x, 0.1f, 0.1f);
	const Vector3 commonLedgeExtentY = Vector3(halfExtent.z, 0.1f, 0.1f);

	AddLedgeToObstacle(ObstacleActor, 
		_pStaticMesh, 
		_pos + Vector3(0.0f, halfExtent.y, halfExtent.z),
		commonLedgeExtentX,
		Quaternion(0.0f, 0.0f, 0.0f, 1.0f)
	);
	AddLedgeToObstacle(ObstacleActor,
		_pStaticMesh,
		_pos + Vector3(0.0f, halfExtent.y, -halfExtent.z),
		commonLedgeExtentX,
		Quaternion(0.0f, 0.0f, 0.0f, 1.0f)
	);
	AddLedgeToObstacle(ObstacleActor,
		_pStaticMesh,
		_pos + Vector3(halfExtent.x, halfExtent.y, 0.0f),
		commonLedgeExtentY,
		Quaternion::CreateFromYawPitchRoll(ToRadians(90.0f), 0.0f, 0.0f)
	);
	AddLedgeToObstacle(ObstacleActor,
		_pStaticMesh,
		_pos + Vector3(-halfExtent.x, halfExtent.y, 0.0f),
		commonLedgeExtentY,
		Quaternion::CreateFromYawPitchRoll(ToRadians(90.0f), 0.0f, 0.0f)
	);

	return ObstacleActor;
}

void TestScene::AddLedgeToObstacle(std::shared_ptr<Actor> _pTarget,
	std::shared_ptr<StaticMesh> _pStaticMesh,
	const Vector3& _localPos,
	const Vector3& _halfExtent,
	const Quaternion& _localRot)
{
	// Ledge 구성
	// box, static rigid body, size, position
	// scene, rigidbody comp
	std::shared_ptr<Physics::PhysicsWorld> phyWorld = GetPhysics().lock();
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
 