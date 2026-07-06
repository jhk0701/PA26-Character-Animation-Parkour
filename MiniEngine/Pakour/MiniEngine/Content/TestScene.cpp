#include "pch.h"
#include "TestScene.h"

#include "Scene/StaticMeshComponent.h"

TestScene::TestScene()
{
}
TestScene::~TestScene()
{
}

void TestScene::Construct()
{
	// 바닥

	// 중간 장애물

	// 사람 크기 장애물

	// 사람보다 큰 장애물
}

std::shared_ptr<MiniEngine::Actor> TestScene::BuildObstacle(const std::wstring& _path)
{
	std::shared_ptr<MiniEngine::Actor> ObstacleActor;

	ObstacleActor = SpawnActor<MiniEngine::Actor>();



	return nullptr;
}
