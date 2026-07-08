#include "pch.h"
#include "TestScene.h"
#include "Manager/PathManager.h"
#include "Manager/AssetManager.h"
#include "Scene/StaticMeshComponent.h"
#include "Scene/RigidBodyComponent.h"
#include "Scene/Tag.h"

#include "Content/Character.h"
#include "Scene/CameraComponent.h"
#include "Scene/SkeletalMeshComponent.h"


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

	{
		std::shared_ptr<Actor> pCamActor = SpawnActor<Actor>();
		pCamActor->SetName("Camera");

		std::shared_ptr<CameraComponent> pCamComp = pCamActor->AddComponent<CameraComponent>();
		pCamComp->aspect = static_cast<float>(WINDOW_WIDTH) / static_cast<float>(WINDOW_HEIGHT);
		pCamComp->RegisterMainCamera();

		pCamActor->GetRoot()->localTransform.position = Vector3(0.0f, 1.0f, -3.0f);
		// m_camController.Initialize(Vector3(0.0f, 0.0f, 0.0f), 10.0f);
	}

	std::shared_ptr<Physics::PhysicsWorld> physics = GetPhysics().lock();

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
		pMeshComp->localTransform.position = Vector3(0.0f, 10.0f, 0.0f);
		pMeshComp->localTransform.rotation = Quaternion::CreateFromYawPitchRoll(30.f, 45.0f, 15.0f);

		std::shared_ptr<RigidBodyComponent> pRB = pCube->AddComponent<RigidBodyComponent>();
		pRB->Init(*physics, RigidBodyComponent::EBodyType::Dynamic, half, 1.f);
		// pRB->SetCollsionGroup(Physics::ECollisionGroup::IgnoreAll); // 충돌그룹 확인용 테스트
	}

	// 중간 장애물
	std::shared_ptr<Actor> pObsMid = BuildObstacle(L"obstacle_mid.mini");
	std::shared_ptr<SceneComponent> pObsMidRoot = pObsMid->GetRoot();
	pObsMidRoot->localTransform.position = Vector3(0.0f, 0.0f, 0.0f);

	// 사람 크기 장애물
	std::shared_ptr<Actor> pObsTop = BuildObstacle(L"obstacle_top.mini");
	std::shared_ptr<SceneComponent> pObsTopRoot = pObsTop->GetRoot();
	pObsTopRoot->localTransform.position = Vector3(0.0f, 0.0f, 5.0f);

	// 사람보다 큰 장애물
	std::shared_ptr<Actor> pObsHigh = BuildObstacle(L"obstacle_high.mini");
	std::shared_ptr<SceneComponent> pObsHighRoot = pObsHigh->GetRoot();
	pObsHighRoot->localTransform.position = Vector3(0.0f, 0.0f, 10.0f);

	// 임시 캐릭터 생성
	{
		PathManager* pathMgr = PathManager::GetInstance();

		std::wstring miniPath = pathMgr->ResolveAssetPath(L"YBot.mini");
		std::shared_ptr<MiniEngine::SkinnedMesh> skinnedMesh = AssetManager::GetInstance()->LoadSkinnedMesh(miniPath);

		std::shared_ptr<Character> pChar = SpawnActor<Character>();
		std::shared_ptr<SkeletalMeshComponent> skinComp = pChar->AddComponent<SkeletalMeshComponent>();
		skinComp->SetMesh(skinnedMesh);
		skinComp->SetActiveClip(2);

		std::shared_ptr<SceneComponent> charRoot = pChar->GetRoot();
		charRoot->localTransform.position = Vector3(0.0f);
		charRoot->localTransform.rotation = Quaternion::CreateFromYawPitchRoll(ToRadians(180.0f), 0.0f, 0.0f);
	}
}

std::shared_ptr<Actor> TestScene::BuildObstacle(const wchar_t* _path)
{
	std::wstring assetPath = PathManager::GetInstance()->ResolveAssetPath(_path);
	std::shared_ptr<StaticMesh> pMesh = AssetManager::GetInstance()->LoadStaticMesh(assetPath);

	if (pMesh == nullptr)
		return nullptr;

	std::shared_ptr<Actor> ObstacleActor;
	ObstacleActor = SpawnActor<Actor>();

	std::shared_ptr<StaticMeshComponent> staticMeshComp = ObstacleActor->AddComponent<StaticMeshComponent>();
	staticMeshComp->SetMesh(pMesh);

	return ObstacleActor;
}
