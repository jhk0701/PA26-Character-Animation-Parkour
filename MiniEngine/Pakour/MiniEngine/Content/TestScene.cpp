#include "pch.h"
#include "TestScene.h"
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

	physics->ToggleDebugMode(true);

	std::shared_ptr<StaticMesh> pCubeMesh;
	std::wstring assetPath = PathManager::GetInstance()->ResolveAssetPath(L"Cube.mini");
	pCubeMesh = AssetManager::GetInstance()->LoadStaticMesh(assetPath);

	{
		// 강체 바닥 설치
		const Vector3 half(100.0f, 0.5f, 100.0f);
		std::shared_ptr<Actor> pGround = SpawnActor<Actor>();
		pGround->SetName("Ground");

		std::shared_ptr<StaticMeshComponent> pMeshComp = pGround->AddComponent<StaticMeshComponent>();
		pMeshComp->SetMesh(pCubeMesh);
		pMeshComp->localTransform.position = Vector3(0.0f, -0.5f, 0.0f);
		pMeshComp->localTransform.scale = half;

		std::shared_ptr<RigidBodyComponent> pRB = pGround->AddComponent<RigidBodyComponent>();
		pRB->Init(*physics, RigidBodyComponent::EBodyType::Static, half);
	}

	{
		// 낙하할 큐브 생성
		const Vector3 half(1.0f, 1.0f, 1.0f);
		std::shared_ptr<Actor> pCube = SpawnActor<Actor>();
		pCube->SetName("Falling Box");

		std::shared_ptr<StaticMeshComponent> pMeshComp = pCube->AddComponent<StaticMeshComponent>();
		pMeshComp->SetMesh(pCubeMesh);
		pMeshComp->localTransform.position = Vector3(5.0f, 10.0f, 0.0f);
		pMeshComp->localTransform.rotation = Quaternion::CreateFromYawPitchRoll(30.f, 45.0f, 15.0f);

		std::shared_ptr<RigidBodyComponent> pRB = pCube->AddComponent<RigidBodyComponent>();
		pRB->Init(*physics, RigidBodyComponent::EBodyType::Dynamic, half, 1.f);
		// pRB->SetCollsionGroup(Physics::ECollisionGroup::IgnoreAll); // 충돌그룹 확인용 테스트
	}

	{	
		// 중간 장애물
		Vector3 scale(2.0f, 1.0f, 0.5f);
		std::shared_ptr<Actor> pObsMid = BuildObstacle(L"Cube.mini", scale * 0.5f);
		std::shared_ptr<SceneComponent> pObsMidRoot = pObsMid->GetRoot();
		pObsMidRoot->localTransform.position = Vector3(0.0f, 0.5f, 0.0f);
		

		// 사람 크기 장애물
		scale = {2.0f, 2.0f, 0.5f};
		std::shared_ptr<Actor> pObsHead = BuildObstacle(L"Cube.mini", scale * 0.5f);
		std::shared_ptr<SceneComponent> pObsHeadRoot = pObsHead->GetRoot();
		pObsHeadRoot->localTransform.position = Vector3(0.0f, 1.0f, 2.0f);

		// 사람보다 큰 장애물
		scale = { 4.0f, 4.0f, 0.5f };
		std::shared_ptr<Actor> pObsOverHead = BuildObstacle(L"Cube.mini", scale * 0.5f);
		std::shared_ptr<SceneComponent> pObsOverHeadRoot = pObsOverHead->GetRoot();
		pObsOverHeadRoot->localTransform.position = Vector3(0.0f, 2.0f, 4.0f);
	}

	{
		// 임시 캐릭터 생성
		std::shared_ptr<Character> pChar = SpawnActor<Character>();
		pChar->Construct();
	}
}

void TestScene::BeginPlay()
{
	Scene::BeginPlay();
}

std::shared_ptr<Actor> TestScene::BuildObstacle(const wchar_t* _path, const Vector3& _scale)
{
	std::wstring assetPath = PathManager::GetInstance()->ResolveAssetPath(_path);
	std::shared_ptr<StaticMesh> pMesh = AssetManager::GetInstance()->LoadStaticMesh(assetPath);

	if (pMesh == nullptr)
		return nullptr;

	std::shared_ptr<Actor> ObstacleActor;
	ObstacleActor = SpawnActor<Actor>();

	std::shared_ptr<StaticMeshComponent> staticMeshComp = ObstacleActor->AddComponent<StaticMeshComponent>();
	staticMeshComp->SetMesh(pMesh);
	staticMeshComp->localTransform.scale = _scale;

	std::shared_ptr<Physics::PhysicsWorld> phyWorld = GetPhysics().lock();

	std::shared_ptr<RigidBodyComponent> rbComp = ObstacleActor->AddComponent<RigidBodyComponent>();
	rbComp->Init(*phyWorld, RigidBodyComponent::EBodyType::Static, _scale);

	return ObstacleActor;
}
 