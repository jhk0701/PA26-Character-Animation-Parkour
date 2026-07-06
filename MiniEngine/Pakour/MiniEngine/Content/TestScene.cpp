#include "pch.h"
#include "TestScene.h"
#include "Manager/PathManager.h"
#include "Manager/AssetManager.h"
#include "Scene/StaticMeshComponent.h"

using namespace MiniEngine;

TestScene::TestScene()
{
}
TestScene::~TestScene()
{
}

void TestScene::Construct()
{
	// 바닥
	std::shared_ptr<Actor> pFloor = BuildObstacle(L"floor.mini");

	// TODO: 메쉬 unit 확인 필요
	// 중간 장애물
	std::shared_ptr<Actor> pObsMid = BuildObstacle(L"obstacle_mid.mini");
	std::shared_ptr<SceneComponent> pObsMidRoot = pObsMid->GetRoot();
	pObsMidRoot->localTransform.position = Vector3(0.0f, 0.0f, -10.0f);
	pObsMidRoot->localTransform.scale = Vector3(0.1f);

	// 사람 크기 장애물
	std::shared_ptr<Actor> pObsTop = BuildObstacle(L"obstacle_top.mini");
	std::shared_ptr<SceneComponent> pObsTopRoot = pObsTop->GetRoot();
	pObsTopRoot->localTransform.position = Vector3(0.0f, 0.0f, -20.0f);
	pObsTopRoot->localTransform.scale = Vector3(0.1f);

	// 사람보다 큰 장애물
	std::shared_ptr<Actor> pObsHigh = BuildObstacle(L"obstacle_high.mini");
	std::shared_ptr<SceneComponent> pObsHighRoot = pObsHigh->GetRoot();
	pObsHighRoot->localTransform.position = Vector3(0.0f, 0.0f, -50.0f);
	pObsHighRoot->localTransform.scale = Vector3(0.1f);
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
