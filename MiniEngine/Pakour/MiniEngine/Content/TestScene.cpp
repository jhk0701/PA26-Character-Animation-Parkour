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
		ObstacleFactory::Create(pScene, pCubeMesh, Vector3(-8.0f, 1.75f, 3.0f), Vector3(3.5f), identity);
		ObstacleFactory::Create(pScene, pCubeMesh, Vector3(-13.0f, 2.5f, 3.0f), Vector3(5.0f), identity);

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
		ObstacleFactory::Create(pScene, pCubeMesh, Vector3(0.0f, 2.0f, 14.0f), Vector3(10.0f, 1.0f, 10.0f), 
			Quaternion::CreateFromYawPitchRoll(0.0f, ToRadians(-30.0f), 0.0f), { (uint8_t)Content::Config::ETagEnv::Obstacle }, false);
		ObstacleFactory::Create(pScene, pCubeMesh, Vector3(0.0f, 2.5f, 20.5f), Vector3(10.0f, 5.0f, 5.0f), identity);
		// 벽
		ObstacleFactory::Create(pScene, pCubeMesh, Vector3(-5.0f, 7.5f, 29.0f), Vector3(0.5f, 8.0f, 20.0f), identity);
		ObstacleFactory::Create(pScene, pCubeMesh, Vector3(5.0f, 7.5f, 29.0f), Vector3(0.5f, 8.0f, 20.0f), identity);

		// Foot Hold // 밟는 용도
		const std::vector<uint8_t> DETAIL_TAGS = 
		{ 
			(uint8_t)Content::Config::ETagEnvDetail::Beam,
			(uint8_t)Content::Config::ETagAxis::X,
		};

		const std::string FOOT_HOLD_NAME = "Foot Hold";
		ObstacleFactory::Create(pScene, pCubeMesh, Vector3(-2.0f, 5.4f, 24.0f), Vector3(3.0f, 0.5f, 0.5f), identity, DETAIL_TAGS)
			->SetName(FOOT_HOLD_NAME.c_str());
		ObstacleFactory::Create(pScene, pCubeMesh, Vector3(-2.0f, 5.8f, 26.0f), Vector3(3.0f, 0.5f, 0.5f), identity, DETAIL_TAGS)
			->SetName(FOOT_HOLD_NAME.c_str());
		ObstacleFactory::Create(pScene, pCubeMesh, Vector3(-2.0f, 5.4f, 28.0f), Vector3(3.0f, 0.5f, 0.5f), identity, DETAIL_TAGS)
			->SetName(FOOT_HOLD_NAME.c_str());
		ObstacleFactory::Create(pScene, pCubeMesh, Vector3(-2.0f, 5.0f, 30.0f), Vector3(4.0f, 0.5f, 0.5f), identity, DETAIL_TAGS)
			->SetName(FOOT_HOLD_NAME.c_str());
		ObstacleFactory::Create(pScene, pCubeMesh, Vector3(-2.0f, 5.4f, 32.0f), Vector3(4.0f, 0.5f, 0.5f), identity, DETAIL_TAGS)
			->SetName(FOOT_HOLD_NAME.c_str());

		// Bar // 잡는 용도
		const std::string BAR_NAME = "Bar";
		ObstacleFactory::Create(pScene, pCubeMesh, Vector3(2.0f, 6.5f, 25.0f), Vector3(4.0f, 0.1f, 0.1f), identity, DETAIL_TAGS)
			->SetName(BAR_NAME.c_str());
		ObstacleFactory::Create(pScene, pCubeMesh, Vector3(2.0f, 6.5f, 27.0f), Vector3(4.0f, 0.1f, 0.1f), identity, DETAIL_TAGS)
			->SetName(BAR_NAME.c_str());
		ObstacleFactory::Create(pScene, pCubeMesh, Vector3(3.0f, 2.5f, 31.0f), Vector3(3.0f, 5.0f, 5.0f), identity);

		// 공중 큐브
		ObstacleFactory::Create(pScene, pCubeMesh, Vector3(0.0f, 28.5f, 0.0f), Vector3(3.0f), identity);
		ObstacleFactory::Create(pScene, pCubeMesh, Vector3(0.0f, 15.0f, 7.0f), Vector3(5.0f), identity);
		ObstacleFactory::Create(pScene, pCubeMesh, Vector3(0.0f, 17.5f, 7.0f), Vector3(5.0f, 1.0f, 0.05f), identity);
	}
	{
		// 임시 캐릭터 생성
		std::shared_ptr<Character> pChar = SpawnActor<Character>();
		pChar->SetName("Character");
		pChar->Construct(Vector3(0.0f, 1.0f, 0.0f));
	}
}

void TestScene::BeginPlay()
{
	Scene::BeginPlay();

#ifdef MG_DEBUG
	ApplyPhysicsDebug(true);
#endif // MG_DEBUG
}
