#include "pch.h"
#include "TestScene.h"

#include "Manager/PathManager.h"
#include "Manager/AssetManager.h"
#include "Manager/DataManager.h"

#include "Scene/StaticMeshComponent.h"
#include "Scene/RigidBodyComponent.h"
#include "Scene/Tag.h"

#include "Perception/Config/ObstacleConfig.h"

#include "Content/ContentConfig.h"
#include "Content/Character.h"
#include "Content/CharacterController.h"
#include "Content/Obstacle.h"

#include "Manager/UIManager.h"
#include "Content/UI/UIDebugBlendClip.h"
#include "Content/UI/UIDebugPerceptionResult.h"
#include "Content/UI/UIDebugMarkerOption.h"
#include "Core/Log.h"

using namespace MiniEngine;

TestScene::TestScene() {}
TestScene::~TestScene() {}

void TestScene::Construct(ID3D11Device* _device, ID3D11DeviceContext* _context)
{
	Scene::Construct(_device, _context);

	std::shared_ptr<Physics::PhysicsWorld> physics = GetPhysics().lock();

	std::shared_ptr<StaticMesh> pCubeMesh;
	std::wstring assetPath = PathManager::GetInstance()->ResolveAssetPath(L"Cube.mini");
	pCubeMesh = AssetManager::GetInstance()->LoadStaticMesh(assetPath);

	std::shared_ptr<Scene> pScene = shared_from_this();
	Quaternion identity = Quaternion(0.0f, 0.0f, 0.0f, 1.0f);

	// 강체 바닥
	{
		ObstacleDesc desc;
		desc.pMesh = pCubeMesh;
		desc.color = Vector3(0.5f, 0.5f, 0.5f);
		desc.pos = Vector3(0.0f, -0.25f, 0.0f);
		desc.scale = Vector3(150.0f, 0.5f, 150.0f);
		desc.layer = MiniEngine::Physics::Layer::Ground;
		// desc.ledgeOpt = Obstacle::ELedgeOption::None;

		std::shared_ptr<Actor> pGround = ObstacleFactory::Create(pScene, desc);
		pGround->SetName("Ground");
	}
	{
		ObstacleDesc desc;
		desc.pMesh = pCubeMesh;
		desc.layer = MiniEngine::Physics::Layer::Obstacle;
		// desc.ledgeOpt = Obstacle::ELedgeOption::All;

		// 1. mantle
		desc.pos = Vector3(-5.0f, 1.25f, 3.0f);
		desc.scale = Vector3(2.5f);
		ObstacleFactory::Create(pScene, desc);

		// 2. mantle 3.0
		desc.pos = Vector3(-8.0f, 1.75f, 3.0f);
		desc.scale = Vector3(3.5f);
		ObstacleFactory::Create(pScene, desc);
		desc.pos = Vector3(-13.0f, 2.5f, 3.0f);
		desc.scale = Vector3(5.0f);
		ObstacleFactory::Create(pScene, desc);

		// 2. vault
		desc.pos = Vector3(0.0f, 0.5f, 3.0f);
		desc.scale = Vector3(4.0f, 1.0f, 0.5f);
		ObstacleFactory::Create(pScene, desc);

		desc.pos = Vector3(-3.0f, 0.5f, 0.0f);
		desc.scale = Vector3(4.0f, 1.0f, 0.5f);
		desc.rot = Quaternion::CreateFromYawPitchRoll(ToRadians(90.0f), 0.0f, 0.0f);
		ObstacleFactory::Create(pScene, desc);

		desc.rot = Quaternion(0.0f, 0.0f, 0.0f, 1.0f);

		// 2-2. valut complex
		// celing 
		desc.pos = Vector3(0.0f, 0.5f, -5.0f);
		desc.scale = Vector3(4.0f, 1.0f, 0.5f);
		ObstacleFactory::Create(pScene, desc);

		desc.pos = Vector3(0.0f, 2.0f, -6.25f);
		desc.scale = Vector3(4.0f, 1.0f, 3.0f);
		ObstacleFactory::Create(pScene, desc);

		// celing 
		desc.pos = Vector3(-5.0f, 0.5f, -5.0f);
		desc.scale = Vector3(4.0f, 1.0f, 0.5f);
		ObstacleFactory::Create(pScene, desc);

		desc.pos = Vector3(-5.0f, 4.0f, -6.25f);
		desc.scale = Vector3(4.0f, 1.0f, 3.0f);
		ObstacleFactory::Create(pScene, desc);

		desc.pos = Vector3(-10.0f, 0.5f, -5.0f);
		desc.scale = Vector3(4.0f, 1.0f, 0.5f);
		ObstacleFactory::Create(pScene, desc);

		desc.pos = Vector3(-10.0f, 3.0f, -6.25f);
		desc.scale = Vector3(4.0f, 1.0f, 3.0f);
		ObstacleFactory::Create(pScene, desc);


		// 3. mantle -> vault
		desc.pos = Vector3(5.0f, 0.5f, 5.0f);
		desc.scale = Vector3(4.0f, 1.0f, 5.0f);
		ObstacleFactory::Create(pScene, desc);
		desc.pos = Vector3(5.0f, 1.0, 7.5f);
		desc.scale = Vector3(4.0f, 2.0f, 0.5f);
		ObstacleFactory::Create(pScene, desc);

		// 4. mantle -> mantle
		desc.pos = Vector3(10.0f, 0.5f, 5.0f);
		desc.scale = Vector3(4.0f, 1.0f, 5.0f);
		ObstacleFactory::Create(pScene, desc);
		desc.pos = Vector3(10.0f, 1.5f, 6.0f);
		desc.scale = Vector3(4.0f, 4.0f, 3.0f);
		ObstacleFactory::Create(pScene, desc);
		desc.pos = Vector3(10.0f, 5.0f, 7.25f);
		desc.scale = Vector3(4.0f, 1.0f, 0.5f);
		ObstacleFactory::Create(pScene, desc);

		// 5. hanging 벽
		desc.pos = Vector3(15.0f, 3.5f, 6.0f);
		desc.scale = Vector3(4.0f, 7.0f, 3.0f);
		ObstacleFactory::Create(pScene, desc);
		// 5-1. 벽 - 벽
		desc.pos = Vector3(20.0f, 3.5f, 8.0f);
		desc.scale = Vector3(6.0f, 7.0f, 3.0f);
		ObstacleFactory::Create(pScene, desc);
		// 5-2. 벽 - 지붕
		desc.pos = Vector3(20.0f, 5.0f, 6.0f);
		desc.scale = Vector3(6.0f, 0.05f, 1.0f);
		ObstacleFactory::Create(pScene, desc);

		// 벽면 돌출물 설치
		desc.color += Vector3(0.3f, -0.2f, 0.0f);
		desc.scale = Vector3(1.0f, 0.2f, 0.2f);
		// desc.ledgeOpt = Obstacle::ELedgeOption::Single;
		desc.tagEnvDetail = (uint8_t)ETagEnvDetail::Protrude;

		desc.pos = Vector3(14.0f, 4.0f, 4.5f);
		ObstacleFactory::Create(pScene, desc)->SetName("Protrude 1");

		desc.pos = Vector3(15.0f, 5.0f, 4.5f);
		ObstacleFactory::Create(pScene, desc)->SetName("Protrude 2");

		desc.pos = Vector3(16.0f, 6.0f, 4.5f);
		ObstacleFactory::Create(pScene, desc)->SetName("Protrude 3");
	}
	{
		ObstacleDesc desc;
		desc.pMesh = pCubeMesh;
		desc.layer = MiniEngine::Physics::Layer::Obstacle;
		// desc.ledgeOpt = Obstacle::ELedgeOption::All;

		// 6. 건물
		desc.pos = Vector3(0.0f, 2.5f, 20.5f);
		desc.scale = Vector3(10.0f, 5.0f, 5.0f);
		ObstacleFactory::Create(pScene, desc);
		// 벽
		desc.pos = Vector3(-5.0f, 7.5f, 29.0f);
		desc.scale = Vector3(0.5f, 8.0f, 20.0f);
		ObstacleFactory::Create(pScene, desc);

		desc.pos = Vector3(5.0f, 7.5f, 29.0f);
		desc.scale = Vector3(0.5f, 16.0f, 16.0f);
		ObstacleFactory::Create(pScene, desc);

		// 중간 지붕 형태
		desc.pos = Vector3(3.5f, 7.5f, 35.0f);
		desc.scale = Vector3(3.0f, 0.25f, 5.0f);
		// desc.ledgeOpt = Obstacle::ELedgeOption::Single;
		ObstacleFactory::Create(pScene, desc);

		// 경사로
		desc.pos = Vector3(0.0f, 2.0f, 14.0f);
		desc.scale = Vector3(10.0f, 1.0f, 10.0f);
		desc.rot = Quaternion::CreateFromYawPitchRoll(0.0f, ToRadians(-30.0f), 0.0f);
		// desc.ledgeOpt = Obstacle::ELedgeOption::Single;
		ObstacleFactory::Create(pScene, desc);
	}
	{
		ObstacleDesc desc;
		desc.pMesh = pCubeMesh;
		desc.layer = MiniEngine::Physics::Layer::Obstacle;
		// desc.ledgeOpt = Obstacle::ELedgeOption::Single;
		desc.tagEnvDetail = (uint8_t)ETagEnvDetail::Beam;
		desc.tagEnvSubInfo = (uint8_t)ETagAxis::X;

		// Foot Hold // 밟는 용도
		const std::string FOOT_HOLD_NAME = "Foot Hold";

		desc.pos = Vector3(-3.0f, 5.4f, 24.0f);
		desc.scale = Vector3(4.0f, 0.5f, 0.5f);
		ObstacleFactory::Create(pScene, desc)->SetName(FOOT_HOLD_NAME.c_str());

		desc.pos = Vector3(-3.0f, 5.8f, 26.0f);
		desc.scale = Vector3(4.0f, 0.5f, 0.5f);
		ObstacleFactory::Create(pScene, desc)->SetName(FOOT_HOLD_NAME.c_str());

		desc.pos = Vector3(-3.0f, 5.4f, 28.0f);
		desc.scale = Vector3(4.0f, 0.5f, 0.5f);
		ObstacleFactory::Create(pScene, desc)->SetName(FOOT_HOLD_NAME.c_str());

		desc.pos = Vector3(-3.0f, 5.0f, 30.0f);
		desc.scale = Vector3(4.0f, 0.5f, 0.5f);
		ObstacleFactory::Create(pScene, desc)->SetName(FOOT_HOLD_NAME.c_str());

		desc.pos = Vector3(-3.0f, 5.4f, 32.0f);
		desc.scale = Vector3(4.0f, 0.5f, 0.5f);
		ObstacleFactory::Create(pScene, desc)->SetName(FOOT_HOLD_NAME.c_str());

		desc.pos = Vector3(-4.25, 7.0f, 22.0f);
		desc.scale = Vector3(1.0f, 0.25f, 0.25f);
		ObstacleFactory::Create(pScene, desc)->SetName(FOOT_HOLD_NAME.c_str());

		desc.pos = Vector3(-4.25f, 8.0f, 23.0f);
		desc.scale = Vector3(1.0f, 0.25f, 0.25f);
		ObstacleFactory::Create(pScene, desc)->SetName(FOOT_HOLD_NAME.c_str());

		desc.pos = Vector3(-4.25, 9.0f, 24.0f);
		desc.scale = Vector3(1.0f, 0.25f, 0.25f);
		ObstacleFactory::Create(pScene, desc)->SetName(FOOT_HOLD_NAME.c_str());
		desc.rot = identity;

		// Bar // 잡는 용도
		const std::string BAR_NAME = "Bar";

		desc.pos = Vector3(3.0f, 6.5f, 25.0);
		desc.scale = Vector3(4.0f, 0.1f, 0.1f);
		ObstacleFactory::Create(pScene, desc)->SetName(BAR_NAME.c_str());

		desc.pos = Vector3(3.0f, 6.5f, 27.0f);
		desc.scale = Vector3(4.0f, 0.1f, 0.1f);
		ObstacleFactory::Create(pScene, desc)->SetName(BAR_NAME.c_str());

		desc.pos = Vector3(3.0f, 9.5f, 26.0f);
		desc.scale = Vector3(4.0f, 0.1f, 0.1f);
		ObstacleFactory::Create(pScene, desc)->SetName(BAR_NAME.c_str());

		desc.tagEnvDetail = (uint8_t)ETagEnvDetail::Default;
		desc.pos = Vector3(3.0f, 2.5f, 31.0f);
		desc.scale = Vector3(3.0f, 5.0f, 5.0f);
		ObstacleFactory::Create(pScene, desc);
	}
	{
		ObstacleDesc desc;
		desc.pMesh = pCubeMesh;
		desc.layer = MiniEngine::Physics::Layer::Obstacle;
		// desc.ledgeOpt = Obstacle::ELedgeOption::All;

		// 공중 큐브
		desc.pos = Vector3(0.0f, 28.5f, 0.0f);
		desc.scale = Vector3(3.0f);
		ObstacleFactory::Create(pScene, desc);

		desc.pos = Vector3(0.0f, 15.0f, 10.0f);
		desc.scale = Vector3(5.0f, 5.0f, 0.5f);
		ObstacleFactory::Create(pScene, desc);
	}
	{
		// 복합 지형 2.
		const Vector3 OFFSET(15.0f, 0.0f, 20.0f);

		ObstacleDesc desc;
		desc.pMesh = pCubeMesh;
		desc.layer = MiniEngine::Physics::Layer::Obstacle;
		// desc.ledgeOpt = Obstacle::ELedgeOption::Single;
		
		{
			// 구간 1
			// 벽 타기 구간
			desc.pos = OFFSET + Vector3(0.0f, 5.0f, 0.0f);
			desc.scale = Vector3(7.0f, 10.0f, 10.0f);
			ObstacleFactory::Create(pScene, desc);

			desc.pos = OFFSET + Vector3(10.0f, 5.0f, 0.0f);
			desc.scale = Vector3(7.0f, 10.0f, 10.0f);
			ObstacleFactory::Create(pScene, desc);

			desc.pos = OFFSET + Vector3(8.5f, 5.0f, 5.0f);
			desc.scale = Vector3(17.0f, 10.0f, 5.0f);
			ObstacleFactory::Create(pScene, desc);

			desc.pos = OFFSET + Vector3(-1.5f, 7.0f, -5.5f);
			desc.scale = Vector3(3.0f, 3.0f, 3.0f);
			ObstacleFactory::Create(pScene, desc);

			desc.pos = OFFSET + Vector3(8.0f, 10.0f, 5.0f);
			desc.scale = Vector3(0.5f, 2.0f, 5.0f);
			ObstacleFactory::Create(pScene, desc);

			desc.pos = OFFSET + Vector3(15.0f, 10.0f, 5.0f);
			desc.scale = Vector3(6.0f, 5.0f, 5.0f);
			ObstacleFactory::Create(pScene, desc);

			desc.pos = OFFSET + Vector3(17.25f, 13.0f, 5.0f);
			desc.scale = Vector3(0.5f, 1.0f, 5.0f);
			ObstacleFactory::Create(pScene, desc);
		}
		{
			// 구간 2
			desc.pos = OFFSET + Vector3(25.0f, 3.0f, 5.0f);
			desc.scale = Vector3(10.0f, 6.0f, 5.0f);
			ObstacleFactory::Create(pScene, desc);

			desc.pos = OFFSET + Vector3(30.0f, 7.5f, -2.5f);
			desc.scale = Vector3(3.0f, 15.0f, 10.0f);
			ObstacleFactory::Create(pScene, desc);

			desc.pos = OFFSET + Vector3(20.0f, 5.0f, -2.5f);
			desc.scale = Vector3(3.0f, 10.0f, 10.0f);
			ObstacleFactory::Create(pScene, desc);

			desc.pos = OFFSET + Vector3(21.4f, 10.5f, -2.5f);
			desc.scale = Vector3(0.25f, 1.0f, 10.0f);
			ObstacleFactory::Create(pScene, desc);
		}

		desc.pos = OFFSET + Vector3(21.0f, 5.0f, -15.0f);
		desc.scale = Vector3(7.0f, 10.0f, 5.0f);
		ObstacleFactory::Create(pScene, desc);

		desc.pos = OFFSET + Vector3(15.0f, 3.5f, -15.0f);
		desc.scale = Vector3(5.0f, 7.0f, 5.0f);
		ObstacleFactory::Create(pScene, desc);

		desc.pos = OFFSET + Vector3(15.0f, 6.0f, -10.0f);
		desc.scale = Vector3(5.0f, 1.0f, 5.0f);
		ObstacleFactory::Create(pScene, desc);

		desc.pos = OFFSET + Vector3(15.0f, 7.5f, -12.75f);
		desc.scale = Vector3(5.0f, 1.0f, 0.5f);
		ObstacleFactory::Create(pScene, desc);

		{
			// 구간 1
			// beam 배치
			desc.tagEnvDetail = (uint8_t)ETagEnvDetail::Beam;
			desc.tagEnvSubInfo = (uint8_t)ETagAxis::X;

			desc.color = Vector3(0.25f, 0.25f, 0.75f);
			desc.layer = MiniEngine::Physics::Layer::Obstacle;
			// desc.ledgeOpt = Obstacle::ELedgeOption::Single;

			desc.pos = OFFSET + Vector3(5.0f, 2.5f, -3.0f);
			desc.scale = Vector3(4.0f, 0.1f, 0.1f);
			desc.rot = Quaternion::CreateFromYawPitchRoll(ToRadians(20.0f), 0.0f, 0.0f);
			ObstacleFactory::Create(pScene, desc);

			desc.pos = OFFSET + Vector3(5.0f, 5.0f, -2.0f);
			desc.scale = Vector3(4.0f, 0.1f, 0.1f);
			desc.rot = Quaternion::CreateFromYawPitchRoll(ToRadians(-20.0f), 0.0f, 0.0f);
			ObstacleFactory::Create(pScene, desc);

			desc.pos = OFFSET + Vector3(5.0f, 7.0f, 0.0f);
			desc.scale = Vector3(4.0f, 0.1f, 0.1f);
			desc.rot = Quaternion::CreateFromYawPitchRoll(ToRadians(30.0f), 0.0f, 0.0f);
			ObstacleFactory::Create(pScene, desc);
		}
		{
			// 구간 2
			// beam 배치
			desc.tagEnvDetail = (uint8_t)ETagEnvDetail::Beam;
			desc.tagEnvSubInfo = (uint8_t)ETagAxis::X;

			desc.layer = MiniEngine::Physics::Layer::Obstacle;
			// desc.ledgeOpt = Obstacle::ELedgeOption::Single;
			desc.rot = Quaternion(0.0f, 0.0f, 0.0f, 1.0f);
			
			desc.pos = OFFSET + Vector3(25.0f, 6.7f, 0.5f);
			desc.scale = Vector3(7.0f, 0.1f, 0.1f);
			ObstacleFactory::Create(pScene, desc);

			desc.pos = OFFSET + Vector3(25.0f, 7.0f, -4.0f);
			desc.scale = Vector3(7.0f, 0.1f, 0.1f);
			ObstacleFactory::Create(pScene, desc);

			desc.pos = OFFSET + Vector3(25.0f, 6.5f, -1.5f);
			desc.scale = Vector3(7.0f, 0.1f, 0.1f);
			ObstacleFactory::Create(pScene, desc);

			desc.pos = OFFSET + Vector3(25.0f, 6.5f, -6.0f);
			desc.scale = Vector3(7.0f, 0.1f, 0.1f);
			ObstacleFactory::Create(pScene, desc);

			desc.pos = OFFSET + Vector3(22.5f, 8.5f, -6.0f);
			desc.scale = Vector3(5.0f, 0.1f, 0.1f);
			desc.rot = Quaternion::CreateFromYawPitchRoll(ToRadians(20.0f), 0.0f, 0.0f);
			ObstacleFactory::Create(pScene, desc);

			desc.pos = OFFSET + Vector3(28.5f, 8.5f, -6.0f);
			desc.scale = Vector3(5.0f, 0.1f, 0.1f);
			desc.rot = Quaternion::CreateFromYawPitchRoll(ToRadians(20.0f), 0.0f, 0.0f);
			ObstacleFactory::Create(pScene, desc);

			desc.pos = OFFSET + Vector3(22.5f, 4.5f, -4.0f);
			desc.scale = Vector3(5.0f, 0.1f, 0.1f);
			desc.rot = Quaternion::CreateFromYawPitchRoll(ToRadians(-20.0f), 0.0f, 0.0f);
			ObstacleFactory::Create(pScene, desc);

			desc.pos = OFFSET + Vector3(28.5f, 4.5f, -4.0f);
			desc.scale = Vector3(5.0f, 0.1f, 0.1f);
			desc.rot = Quaternion::CreateFromYawPitchRoll(ToRadians(20.0f), 0.0f, 0.0f);
			ObstacleFactory::Create(pScene, desc);

			desc.pos = OFFSET + Vector3(22.5f, 7.5f, -2.0f);
			desc.scale = Vector3(5.0f, 0.1f, 0.1f);
			desc.rot = Quaternion::CreateFromYawPitchRoll(ToRadians(-20.0f), 0.0f, 0.0f);
			ObstacleFactory::Create(pScene, desc);

			desc.pos = OFFSET + Vector3(28.5f, 7.5f, -2.0f);
			desc.scale = Vector3(5.0f, 0.1f, 0.1f);
			desc.rot = Quaternion::CreateFromYawPitchRoll(ToRadians(-20.0f), 0.0f, 0.0f);
			ObstacleFactory::Create(pScene, desc);

			desc.rot = Quaternion(0.0f, 0.0f, 0.0f, 1.0f);
		}

		{
			// Protrude 배치
			desc.tagEnvDetail = (uint8_t)ETagEnvDetail::Protrude;
			desc.tagPriority = 1U;

			desc.color = Vector3(0.75f, 0.25f, 0.25f);
			desc.layer = MiniEngine::Physics::Layer::Obstacle;
			// desc.ledgeOpt = Obstacle::ELedgeOption::Single;

			// 구간 1
			desc.pos = OFFSET + Vector3(1.5f, 4.0f, -5.0f);
			desc.scale = Vector3(1.0f, 0.2f, 0.4f);
			ObstacleFactory::Create(pScene, desc)->SetName("Protrude 1-1");

			desc.pos = OFFSET + Vector3(1.9f, 6.0f, -5.0f);
			desc.scale = Vector3(1.0f, 0.2f, 0.4f);
			ObstacleFactory::Create(pScene, desc)->SetName("Protrude 1-2");

			desc.pos = OFFSET + Vector3(1.4f, 8.0f, -5.0f);
			desc.scale = Vector3(1.0f, 0.2f, 0.4f);
			ObstacleFactory::Create(pScene, desc)->SetName("Protrude 1-3");


			// 구간 3
			desc.pos = OFFSET + Vector3(19.0f, 7.0f, -12.55f);
			desc.scale = Vector3(1.0f, 0.2f, 0.5f);
			ObstacleFactory::Create(pScene, desc)->SetName("Protrude 3-1");

			desc.pos = OFFSET + Vector3(21.0f, 6.0f, -12.55f);
			desc.scale = Vector3(1.0f, 0.2f, 0.5f);
			ObstacleFactory::Create(pScene, desc)->SetName("Protrude 3-2");

			desc.pos = OFFSET + Vector3(23.0f, 7.0f, -12.55f);
			desc.scale = Vector3(1.0f, 0.2f, 0.5f);
			ObstacleFactory::Create(pScene, desc)->SetName("Protrude 3-3");

			desc.pos = OFFSET + Vector3(19.0f, 5.0f, -12.55f);
			desc.scale = Vector3(1.0f, 0.2f, 0.5f);
			ObstacleFactory::Create(pScene, desc)->SetName("Protrude 3-4");

			desc.pos = OFFSET + Vector3(21.0f, 4.0f, -12.55f);
			desc.scale = Vector3(1.0f, 0.2f, 0.5f);
			ObstacleFactory::Create(pScene, desc)->SetName("Protrude 3-5");

			desc.pos = OFFSET + Vector3(23.0f, 5.0f, -12.55f);
			desc.scale = Vector3(1.0f, 0.2f, 0.5f);
			ObstacleFactory::Create(pScene, desc)->SetName("Protrude 3-6");

			desc.pos = OFFSET + Vector3(19.0f, 3.0f, -12.55f);
			desc.scale = Vector3(1.0f, 0.2f, 0.5f);
			ObstacleFactory::Create(pScene, desc)->SetName("Protrude 3-7");

			desc.pos = OFFSET + Vector3(21.0f, 2.0f, -12.55f);
			desc.scale = Vector3(1.0f, 0.2f, 0.5f);
			ObstacleFactory::Create(pScene, desc)->SetName("Protrude 3-8");

			desc.pos = OFFSET + Vector3(23.0f, 3.0f, -12.55f);
			desc.scale = Vector3(1.0f, 0.2f, 0.5f);
			ObstacleFactory::Create(pScene, desc)->SetName("Protrude 3-9");

			desc.tagPriority = 0U;
		}
	}
	{
		const Vector3 OFFSET(-10.0f, 0.0f, 0.0f);
		{
			// 절벽
			ObstacleDesc desc;
			desc.pMesh = pCubeMesh;
			desc.layer = MiniEngine::Physics::Layer::Obstacle;
			// desc.ledgeOpt = Obstacle::ELedgeOption::Single;
		
			desc.pos = OFFSET + Vector3(-20.0f, 25.0f, 0.0f);
			desc.scale = Vector3(3.0f, 50.0f, 100.0f);
			ObstacleFactory::Create(pScene, desc)->SetName("Cliff");

			desc.color = Vector3(0.5f, 0.5f, 0.1f);

			desc.pos = OFFSET + Vector3(-17.5f, 9.0f, -6.0f);
			desc.scale = Vector3(3.0f, 3.0f, 8.0f);
			ObstacleFactory::Create(pScene, desc)->SetName("Cliff Default 1");

			desc.pos = OFFSET + Vector3(-17.5f, 17.0f, 2.25f);
			desc.scale = Vector3(3.0f, 2.0f, 0.5f);
			ObstacleFactory::Create(pScene, desc)->SetName("Cliff Vault High 1");

			desc.pos = OFFSET + Vector3(-17.5f, 13.0f, -2.0f);
			desc.scale = Vector3(3.0f, 2.0f, 3.0f);
			ObstacleFactory::Create(pScene, desc)->SetName("Cliff Default Air Mantle");

			desc.pos = OFFSET + Vector3(-17.5f, 15.0f, 6.0f);
			desc.scale = Vector3(3.0f, 3.0f, 8.0f);
			ObstacleFactory::Create(pScene, desc)->SetName("Cliff Default 2");

			desc.pos = OFFSET + Vector3(-18.5f, 5.0f, 2.0f);
			desc.scale = Vector3(3.0f, 3.0f, 3.0f);
			ObstacleFactory::Create(pScene, desc)->SetName("Cliff Default 3");

			desc.pos = OFFSET + Vector3(-18.5f, 3.0f, -4.0f);
			desc.scale = Vector3(3.0f, 3.0f, 3.0f);
			ObstacleFactory::Create(pScene, desc)->SetName("Cliff Default 4");

			{
				// 돌출부 배치
				desc.tagEnvDetail = (uint8_t)ETagEnvDetail::Protrude;
				desc.tagEnvSubInfo = (uint8_t)ETagAxis::X;
				desc.tagPriority = 1U;
				desc.color = Vector3(1.0f, 0.1f, 0.1f);

				desc.pos = OFFSET + Vector3(-18.5f, 5.0f, 11.0f);
				desc.scale = Vector3(0.2f, 0.1f, 0.7f);
				ObstacleFactory::Create(pScene, desc)->SetName("Cliff Protrude 1");

				desc.pos = OFFSET + Vector3(-18.5f, 6.0f, 14.0f);
				desc.scale = Vector3(0.2f, 0.1f, 0.7f);
				ObstacleFactory::Create(pScene, desc)->SetName("Cliff Protrude 2");

				desc.pos = OFFSET + Vector3(-18.5f, 7.5f, 13.5f);
				desc.scale = Vector3(0.2f, 0.1f, 0.7f);
				ObstacleFactory::Create(pScene, desc)->SetName("Cliff Protrude 3");

				desc.pos = OFFSET + Vector3(-18.5f, 9.0f, 14.0f);
				desc.scale = Vector3(0.2f, 0.1f, 0.7f);
				ObstacleFactory::Create(pScene, desc)->SetName("Cliff Protrude 4");

				desc.pos = OFFSET + Vector3(-18.5f, 10.5f, 13.5f);
				desc.scale = Vector3(0.2f, 0.1f, 0.7f);
				ObstacleFactory::Create(pScene, desc)->SetName("Cliff Protrude 5");
			}

		}
		{
			// 나무 등 기둥
			ObstacleDesc desc;
			desc.pMesh = pCubeMesh;

			desc.tagEnvDetail = (uint8_t)ETagEnvDetail::Pole;
			desc.tagEnvSubInfo = (uint8_t)ETagAxis::X; // 특정 축으로 매달리기
			desc.tagPriority = 1U; // 한 단계 높은 우선순위

			desc.layer = MiniEngine::Physics::Layer::Obstacle;
			desc.color = Vector3(0.75f, 0.75f, 0.25f);
			desc.meshPos = Vector3(0.0f);

			desc.pos = OFFSET + Vector3(-18.4f, 3.0f, 6.0f);
			desc.scale = Vector3(0.1f, 6.0f, 0.1f);
			ObstacleFactory::Create(pScene, desc)->SetName("Pole 1");

			desc.pos = OFFSET + Vector3(-18.4f, 6.0f, 8.0f);
			desc.scale = Vector3(0.1f, 8.0f, 0.1f);
			ObstacleFactory::Create(pScene, desc)->SetName("Pole 2");

			desc.pos = OFFSET + Vector3(-18.4f, 8.0f, 10.0f);
			desc.scale = Vector3(0.1f, 10.0f, 0.1f);
			ObstacleFactory::Create(pScene, desc)->SetName("Pole 3");

			desc.pos = OFFSET + Vector3(-18.4f, 14.0f, 12.0f);
			desc.scale = Vector3(0.1f, 14.0f, 0.1f);
			ObstacleFactory::Create(pScene, desc)->SetName("Pole 4");
		}
		{
			// 비정형 나무 
			ObstacleDesc desc;
			desc.pMesh = pCubeMesh;
			desc.layer = MiniEngine::Physics::Layer::Obstacle;
			desc.color = Vector3(0.75f, 0.75f, 0.25f);

			desc.tagEnvDetail = (uint8_t)ETagEnvDetail::Pole;
			desc.tagPriority = 1U; // 한 단계 높은 우선순위

			desc.pos = OFFSET + Vector3(-15.0f, 5.0f, -5.0f);
			desc.scale = Vector3(0.1f, 10.0f, 0.1f);
			ObstacleFactory::Create(pScene, desc)->SetName("Pole Tree 1");

			desc.pos = OFFSET + Vector3(-15.0f, 5.0f, -10.0f);
			desc.scale = Vector3(0.1f, 10.0f, 0.1f);
			ObstacleFactory::Create(pScene, desc)->SetName("Pole Tree 2");

			desc.tagEnvDetail = (uint8_t)ETagEnvDetail::Beam;
			desc.tagEnvSubInfo = (uint8_t)ETagAxis::X;

			desc.pos = OFFSET + Vector3(-15.0f, 9.0f, -5.0f);
			desc.scale = Vector3(4.0f, 0.2f, 0.2f);
			desc.meshPos = Vector3(2.0f, 0.0f, 0.0f);
			ObstacleFactory::Create(pScene, desc)->SetName("Beam Branch 1");

			desc.pos = OFFSET + Vector3(-15.0f, 9.0f, -5.0f);
			desc.scale = Vector3(4.0f, 0.2f, 0.2f);
			desc.meshPos = Vector3(2.0f, 0.0f, 0.0f);
			desc.rot = Quaternion::CreateFromYawPitchRoll(ToRadians(45.0f), 0.0f, 0.0f);
			ObstacleFactory::Create(pScene, desc)->SetName("Beam Branch 2");

			desc.pos = OFFSET + Vector3(-15.0f, 9.0f, -5.0f);
			desc.scale = Vector3(4.0f, 0.2f, 0.2f);
			desc.meshPos = Vector3(2.0f, 0.0f, 0.0f);
			desc.rot = Quaternion::CreateFromYawPitchRoll(ToRadians(90.0f), 0.0f, 0.0f);
			ObstacleFactory::Create(pScene, desc)->SetName("Beam Branch 3");

			desc.pos = OFFSET + Vector3(-15.0f, 6.0f, -5.0f);
			desc.scale = Vector3(4.0f, 0.2f, 0.2f);
			desc.meshPos = Vector3(2.0f, 0.0f, 0.0f);
			desc.rot = Quaternion::CreateFromYawPitchRoll(ToRadians(30.0f), 0.0f, 0.0f);
			ObstacleFactory::Create(pScene, desc)->SetName("Beam Branch 4");

			desc.pos = OFFSET + Vector3(-15.0f, 4.0f, -5.0f);
			desc.scale = Vector3(5.0f, 0.2f, 0.2f);
			desc.meshPos = Vector3(2.5f, 0.0f, 0.0f);
			desc.rot = Quaternion::CreateFromYawPitchRoll(ToRadians(90.0f), 0.0f, 0.0f);
			ObstacleFactory::Create(pScene, desc)->SetName("Beam Branch 5");
		}
		{
			ObstacleDesc desc;
			desc.pMesh = pCubeMesh;
			desc.layer = MiniEngine::Physics::Layer::Obstacle;
			desc.color = Vector3(0.75f, 0.25f, 0.25f);

			desc.pos = OFFSET + Vector3(-15.0f, 7.5f * 0.5f, 0.0f);
			desc.scale = Vector3(0.75f, 7.5f, 0.75f);
			ObstacleFactory::Create(pScene, desc)->SetName("Thick Tree 1");

			desc.pos = OFFSET + Vector3(-16.5f, 6.0f, 0.0f);
			desc.scale = Vector3(3.0f, 0.25f, 3.0f);
			ObstacleFactory::Create(pScene, desc)->SetName("Thick Tree 1 Plate");

			desc.pos = OFFSET + Vector3(-7.5f, 5.0f, 5.0f);
			desc.scale = Vector3(0.75f, 10.0f, 0.75f);
			ObstacleFactory::Create(pScene, desc)->SetName("Thick Tree 2");

			desc.pos = OFFSET + Vector3(-7.5f, 8.0f, 5.0f);
			desc.scale = Vector3(3.0f, 0.25f, 3.0f);
			ObstacleFactory::Create(pScene, desc)->SetName("Thick Tree 2 Plate");

			desc.pos = OFFSET + Vector3(-15.0f, 5.0f, 10.0f);
			desc.scale = Vector3(0.75f, 10.0f, 0.75f);
			ObstacleFactory::Create(pScene, desc)->SetName("Thick Tree 3");

			desc.pos = OFFSET + Vector3(-15.0f, 8.0f, 10.0f);
			desc.scale = Vector3(4.0f, 3.0f, 4.0f);
			ObstacleFactory::Create(pScene, desc)->SetName("Thick Tree 3 Cube");


			desc.tagEnvDetail = (uint8_t)ETagEnvDetail::Beam;
			desc.tagEnvSubInfo = (uint8_t)ETagAxis::X;

			desc.pos = OFFSET + Vector3(-15.0f, 7.5f, 0.0f);
			desc.meshPos = Vector3(4.0f, 0.0f, 0.0f);
			desc.scale = Vector3(8.0f, 0.2f, 0.2f);
			ObstacleFactory::Create(pScene, desc)->SetName("Thick Tree 1 Branch 1");

			desc.pos = OFFSET + Vector3(-15.0f, 7.5f, 0.0f);
			desc.meshPos = Vector3(0.0f, 0.0f, 0.0f);
			desc.scale = Vector3(10.0f, 0.2f, 0.2f);
			desc.rot = Quaternion::CreateFromYawPitchRoll(ToRadians(90.0f), 0.0f, 0.0f);
			ObstacleFactory::Create(pScene, desc)->SetName("Thick Tree 1 Branch 2");
			
			desc.pos = OFFSET + Vector3(-15.0f, 7.5f, 0.0f);
			desc.meshPos = Vector3(4.0f, 0.0f, 0.0f);
			desc.scale = Vector3(8.0f, 0.2f, 0.2f);
			desc.rot = Quaternion::CreateFromYawPitchRoll(ToRadians(90.0f * 3.0f), 0.0f, 0.0f);
			ObstacleFactory::Create(pScene, desc)->SetName("Thick Tree 1 Branch 3");


			desc.meshPos = Vector3(0.0f);
			desc.rot = Quaternion(0.0f, 0.0f, 0.0f, 1.0f);
			desc.color = Vector3(0.25f, 0.75f, 0.25f);
			desc.tagEnvDetail = (uint8_t)ETagEnvDetail::Pole;
			desc.tagEnvSubInfo = (uint8_t)ETagAxis::X;
			desc.tagPriority = 1U;

			desc.pos = OFFSET + Vector3(-12.95f, 3.5f, 8.5f);
			desc.scale = Vector3(0.1f, 7.0f, 0.1f);
			ObstacleFactory::Create(pScene, desc)->SetName("Middle Pole 1");

			desc.pos = OFFSET + Vector3(-12.95f, 6.5f, 10.0f);
			desc.scale = Vector3(0.1f, 6.0f, 0.1f);
			ObstacleFactory::Create(pScene, desc)->SetName("Middle Pole 2");

			desc.pos = OFFSET + Vector3(-12.95f, 3.5f, 11.5f);
			desc.scale = Vector3(0.1f, 7.0f, 0.1f);
			ObstacleFactory::Create(pScene, desc)->SetName("Middle Pole 3");
			
			desc.tagEnvDetail = (uint8_t)ETagEnvDetail::Pole;
			desc.tagEnvSubInfo = (uint8_t)ETagAxis::Z;
			desc.pos = OFFSET + Vector3(-15.0f, 3.5f, 12.05f);
			desc.scale = Vector3(0.1f, 7.0f, 0.1f);
			ObstacleFactory::Create(pScene, desc)->SetName("Middle Pole 4");
		}
		{
			const Vector3 OFFSET = Vector3(-15.0f, 0.0f, -8.0f);
			ObstacleDesc desc;
			desc.pMesh = pCubeMesh;
			desc.layer = MiniEngine::Physics::Layer::Obstacle;

			// 좁은 창틀형
			desc.pos = OFFSET + Vector3(0.0f, 0.5f, 0.0f);
			desc.scale = Vector3(4.0f, 1.0f, 0.5f);
			ObstacleFactory::Create(pScene, desc);

			desc.pos = OFFSET + Vector3(-1.5f, 1.5f, 0.0f);
			desc.scale = Vector3(1.0f, 1.0f, 0.5f);
			ObstacleFactory::Create(pScene, desc);

			desc.pos = OFFSET + Vector3(1.5f, 1.5f, 0.0f);
			desc.scale = Vector3(1.0f, 1.0f, 0.5f);
			ObstacleFactory::Create(pScene, desc);
		}
		{
			const Vector3 OFFSET = Vector3(-15.0f, 0.0f, -13.0f);

			ObstacleDesc desc;
			desc.pMesh = pCubeMesh;
			desc.layer = MiniEngine::Physics::Layer::Obstacle;
			desc.tagPriority = 0U;

			// 좁은 창틀형
			desc.pos = OFFSET + Vector3(0.0f, 0.5f, 0.0f);
			desc.scale = Vector3(4.0f, 1.0f, 0.5f);
			ObstacleFactory::Create(pScene, desc);

			desc.pos = OFFSET + Vector3(-1.5f, 1.5f, 0.0f);
			desc.scale = Vector3(1.0f, 1.5f, 0.5f);
			ObstacleFactory::Create(pScene, desc);

			desc.pos = OFFSET + Vector3(1.5f, 1.5f, 0.0f);
			desc.scale = Vector3(1.0f, 1.5f, 0.5f);
			ObstacleFactory::Create(pScene, desc);

			desc.tagEnvDetail = (uint8_t)ETagEnvDetail::Direct;
			desc.tagPriority = 1U;
			desc.color = Vector3(0.75f, 0.25f, 0.25f);

			desc.pos = OFFSET + Vector3(0.0f, 2.0f, 0.0f);
			desc.scale = Vector3(2.0f, 0.1f, 0.1f);
			ObstacleFactory::Create(pScene, desc, (uint8_t)Content::Config::ETagAct::Direct_Vault_UnderBar)
				->SetName("Directing Under Bar");
		}
	}
	{
		// 캐릭터 생성
		std::shared_ptr<Character> pChar = SpawnActor<Character>();
		pChar->SetName("Character");
		pChar->Construct(Vector3(0.0f, 2.0f, 0.0f), L"Character.mini");

		std::shared_ptr<CharacterController> pCont = SpawnActor<CharacterController>();
		pCont->SetName("CharacterController");
		pCont->Construct();

		pCont->Possess(pChar);

#ifdef MG_DEBUG_UI
		std::shared_ptr<UIDebugPerceptionResult> uiPerceptionResult = UIManager::GetInstance()->CreateUI<UIDebugPerceptionResult>().lock();
		uiPerceptionResult->SetCharacter(pChar);

		std::shared_ptr<UIDebugMarkerOption> uiMarkerOpt = UIManager::GetInstance()->CreateUI<UIDebugMarkerOption>().lock();
		uiMarkerOpt->SetScene(shared_from_this());
#endif 
	}
}

void TestScene::BeginPlay()
{
	Scene::BeginPlay();
}
