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
	if (pFloor == nullptr)
		return;

	// 중간 장애물

	// 사람 크기 장애물

	// 사람보다 큰 장애물
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
