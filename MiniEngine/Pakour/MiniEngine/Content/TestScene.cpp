#include "pch.h"
#include "TestScene.h"

#include "Core/Log.h"
#include "Manager/PathManager.h"
#include "Manager/AssetManager.h"
#include "Manager/DataManager.h"

#include "Scene/StaticMeshComponent.h"
#include "Scene/RigidBodyComponent.h"
#include "Scene/Tag.h"

#include "Content/ContentConfig.h"
#include "Content/Character.h"
#include "Content/Obstacle.h"

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
		std::shared_ptr<Scene> pScene = shared_from_this();
		Quaternion identity = Quaternion(0.0f, 0.0f, 0.0f, 1.0f);

		// 1. mantle
		ObstacleFactory::Create(pScene, pCubeMesh, Vector3(-5.0f, 1.25f, 3.0f), Vector3(2.5f), identity);
		
		// 2. mantle 3.0
		ObstacleFactory::Create(pScene, pCubeMesh, Vector3(-8.0f, 1.5f, 3.0f), Vector3(3.0f), identity);

		// 2. vault
		ObstacleFactory::Create(pScene, pCubeMesh, Vector3(0.0f, 0.5f, 3.0f), Vector3(4.0f, 1.0f, 0.5f), identity);

		// 3. mantle -> vault
		ObstacleFactory::Create(pScene, pCubeMesh, Vector3(5.0f, 0.5f, 5.0f), Vector3(4.0f, 1.0f, 5.0f), identity);
		ObstacleFactory::Create(pScene, pCubeMesh, Vector3(5.0f, 1.0, 7.5f), Vector3(4.0f, 2.0f, 0.5f), identity);

		// 4. mantle -> mantle
		ObstacleFactory::Create(pScene, pCubeMesh, Vector3(10.0f, 0.5f, 5.0f), Vector3(4.0f, 1.0f, 5.0f), identity);
		ObstacleFactory::Create(pScene, pCubeMesh, Vector3(10.0f, 1.5f, 6.0f), Vector3(4.0f, 4.0f, 3.0f), identity);
		ObstacleFactory::Create(pScene, pCubeMesh, Vector3(10.0f, 3.0f, 7.5f), Vector3(4.0f, 1.0f, 0.5f), identity);

		// 5. hanging 벽
		ObstacleFactory::Create(pScene, pCubeMesh, Vector3(15.0f, 0.5f, 5.0f), Vector3(4.0f, 1.0f, 5.0f), identity);
		ObstacleFactory::Create(pScene, pCubeMesh, Vector3(15.0f, 3.5f, 6.0f), Vector3(4.0f, 7.0f, 3.0f), identity);
		// 5-1. 벽 - 벽
		ObstacleFactory::Create(pScene, pCubeMesh, Vector3(20.0f, 3.5f, 8.0f), Vector3(6.0f, 7.0f, 3.0f), identity);
		// 5-2. 벽 - 지붕
		ObstacleFactory::Create(pScene, pCubeMesh, Vector3(20.0f, 5.0f, 6.0f), Vector3(4.0f, 0.05f, 1.0f), identity);

		// 6. 경사로 + 건물
		ObstacleFactory::Create(pScene, pCubeMesh, Vector3(0.0f, 2.0f, 10.0f), Vector3(5.0f, 1.0f, 10.0f), 
			Quaternion::CreateFromYawPitchRoll(0.0f, ToRadians(-30.0f), 0.0f), false);
		ObstacleFactory::Create(pScene, pCubeMesh, Vector3(0.0f, 2.5f, 16.5f), Vector3(5.0f), identity);
		// 벽
		ObstacleFactory::Create(pScene, pCubeMesh, Vector3(-3.0f, 7.5f, 25.0f), Vector3(0.5f, 8.0f, 20.0f), identity);
		ObstacleFactory::Create(pScene, pCubeMesh, Vector3(3.0f, 7.5f, 25.0f), Vector3(0.5f, 8.0f, 20.0f), identity);

		// Foot Hold // 밟는 용도
		uint8_t detailTag = (uint8_t)Content::Config::ETagEnvDetail::FootHold;
		ObstacleFactory::Create(pScene, pCubeMesh, Vector3(-1.1f, 5.0f, 21.0f), Vector3(3.0f, 0.5f, 0.5f), identity, detailTag);
		ObstacleFactory::Create(pScene, pCubeMesh, Vector3(-1.1f, 5.0f, 24.0f), Vector3(3.0f, 0.5f, 0.5f), identity, detailTag);
		ObstacleFactory::Create(pScene, pCubeMesh, Vector3(-1.1f, 5.0f, 28.0f), Vector3(3.0f, 0.5f, 0.5f), identity, detailTag);
		ObstacleFactory::Create(pScene, pCubeMesh, Vector3(1.0f, 6.5f, 29.0f), Vector3(4.0f, 0.5f, 0.5f), identity, detailTag);
		ObstacleFactory::Create(pScene, pCubeMesh, Vector3(1.0f, 8.0f, 31.0f), Vector3(4.0f, 0.5f, 0.5f), identity, detailTag);
		// celing
		ObstacleFactory::Create(pScene, pCubeMesh, Vector3(1.0f, 10.0f, 29.0f), Vector3(4.0f, 0.05f, 4.0f), identity);

		// Bar // 잡는 용도
		detailTag = (uint8_t)Content::Config::ETagEnvDetail::Bar;
		ObstacleFactory::Create(pScene, pCubeMesh, Vector3(1.5f, 6.5f, 34.0f), Vector3(4.0f, 0.1f, 0.1f), identity, detailTag);
		ObstacleFactory::Create(pScene, pCubeMesh, Vector3(1.5f, 6.5f, 36.0f), Vector3(4.0f, 0.1f, 0.1f), identity, detailTag);
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
