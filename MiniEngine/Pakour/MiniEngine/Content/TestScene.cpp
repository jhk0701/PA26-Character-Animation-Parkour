#include "pch.h"
#include "TestScene.h"
#include "Content/ContentConfig.h"
#include "Manager/PathManager.h"
#include "Manager/AssetManager.h"
#include "Scene/StaticMeshComponent.h"
#include "Scene/RigidBodyComponent.h"
#include "Scene/Tag.h"

#include "Content/Character.h"

using namespace MiniEngine;

TestScene::TestScene()
{
}
TestScene::~TestScene()
{
}

void TestScene::Construct()
{
	Scene::Construct();

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
		pRB->Init(*physics, RigidBodyComponent::EBodyType::Static, size, 10.0f, pMeshComp);
		pRB->SetLayer(MiniEngine::Physics::Layer::Ground);
	}

	{	
		// 중간 장애물
		BuildObstacle(L"Cube.mini", Vector3(0.0f, 0.5f, 0.0f), Vector3(2.0f, 1.0f, 0.5f));
		BuildObstacle(L"Cube.mini", Vector3(5.0f, 0.5f, 0.0f), Vector3(2.0f, 1.0f, 5.0f));
		BuildObstacle(L"Cube.mini", Vector3(5.0f, 1.0, 2.0f), Vector3(2.0f, 2.0f, 0.5f));
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
}

std::shared_ptr<Actor> TestScene::BuildObstacle(const wchar_t* _path, const MiniEngine::Vector3& _pos, const Vector3& _scale)
{
	std::wstring assetPath = PathManager::GetInstance()->ResolveAssetPath(_path);
	std::shared_ptr<StaticMesh> pMesh = AssetManager::GetInstance()->LoadStaticMesh(assetPath);

	if (pMesh == nullptr)
		return nullptr;

	std::shared_ptr<Actor> ObstacleActor;
	ObstacleActor = SpawnActor<Actor>();
	ObstacleActor->SetName("Obstacle");
	
	Tag& tag = ObstacleActor->GetTag();
	tag += (uint8_t)Content::Config::ETagEnv::Obstacle;
	tag += (uint8_t)Content::Config::ETagAct::JumpOver;

	std::shared_ptr<StaticMeshComponent> staticMeshComp = ObstacleActor->AddComponent<StaticMeshComponent>();
	staticMeshComp->SetColor(Vector3(0.7f, 0.7f, 0.2f));
	staticMeshComp->SetMesh(pMesh);
	staticMeshComp->localTransform.position = _pos;
	staticMeshComp->localTransform.scale = _scale * 0.5f;

	std::shared_ptr<Physics::PhysicsWorld> phyWorld = GetPhysics().lock();

	std::shared_ptr<RigidBodyComponent> pRB = ObstacleActor->AddComponent<RigidBodyComponent>();
	pRB->Init(*phyWorld, RigidBodyComponent::EBodyType::Static, _scale * 0.5f, 10.0f, staticMeshComp);
	pRB->SetCollsionGroup(MiniEngine::Physics::ECollisionGroup::Obstacle);
	pRB->SetLayer(MiniEngine::Physics::Layer::Obstacle); // 레이캐스트용

	return ObstacleActor;
}
 