#include "pch.h"
#include "Physics/PhysicsWorld.h"

#include "Core/Log.h"
#include <physx/PxPhysicsAPI.h>

using namespace physx;

namespace MiniEngine::Physics 
{
	namespace
	{
		PxDefaultAllocator gAllocator;
		PxDefaultErrorCallback gErrorCallback;

		inline PxVec3 ToPx(const Vector3& v) { return PxVec3(v.x, v.y, v.z); }
		inline PxQuat ToPx(const Quaternion& q) { return PxQuat(q.x, q.y, q.z, q.w); }
	}

	PhysicsWorld::PhysicsWorld() { }
	PhysicsWorld::~PhysicsWorld()
	{
		Shutdown();
	}

	bool PhysicsWorld::Init()
	{
		if (m_scene)
			return true; // 이미 초기화된 상황

		m_foundation = PxCreateFoundation(PX_PHYSICS_VERSION, gAllocator, gErrorCallback);
		if (m_foundation == nullptr)
		{
			MG_LOG_ERROR("physx : PxCreateFoundation failed");
			return false;
		}

		m_physics = PxCreatePhysics(PX_PHYSICS_VERSION, *m_foundation, PxTolerancesScale());
		if (m_physics == nullptr)
		{
			MG_LOG_ERROR("physx : PxCreatePhysics failed");
			Shutdown();
			return false;
		}

		m_dispatcher = PxDefaultCpuDispatcherCreate(1); // 단일 워커 생성

		PxSceneDesc sceneDesc(m_physics->getTolerancesScale());
		sceneDesc.gravity = PxVec3(0.0f, -9.81f, 0.0f);
		sceneDesc.cpuDispatcher = m_dispatcher;
		sceneDesc.filterShader = PxDefaultSimulationFilterShader;
		m_scene = m_physics->createScene(sceneDesc);

		if (!m_scene) 
		{
			MG_LOG_ERROR("physx : createScene failed");
			Shutdown();
			return false;
		}

		// 기본 물리 재질
		m_material = m_physics->createMaterial(0.5f, 0.5f, 0.6f);
		MG_LOG_INFO("PhysX initialized (CPU, gravity -9.81 y-up)");

		SetDefaultCollisionGroup();

		return true;
	}

	void PhysicsWorld::Shutdown()
	{
		// 생성 역순 release. Material 은 m_physics 소유라 별도 release 안 함(참조만 해제).
		if (m_scene) 
		{
			m_scene->release();
			m_scene = nullptr;
		}

		if (m_dispatcher) 
		{
			m_dispatcher->release();
			m_dispatcher = nullptr;
		}

		m_material = nullptr;

		if (m_physics)
		{
			m_physics->release();
			m_physics = nullptr;
		}

		if(m_foundation)
		{
			m_foundation->release();
			m_foundation = nullptr;
		}
	}

	void PhysicsWorld::Step(float _fixedDt)
	{
		if (m_scene == nullptr)
			return;

		m_scene->simulate(_fixedDt);
		m_scene->fetchResults(true); // 블로킹시키고 결과 반영까지 대기
	}

	bool PhysicsWorld::CreateRigidFloor()
	{
		if (m_scene == nullptr)
			return false;

		// 지면 (평면) : 법선 +y, 위치 y = 0, 무한 평면
		// 기본 강체
		PxRigidStatic* ground = PxCreatePlane(*m_physics, PxPlane(0.0f, 1.0f, 0.0f, 0.0f), *m_material);
		if (ground == nullptr)
			return false;

		m_scene->addActor(*ground);
		return true;
	}

	physx::PxRigidActor* PhysicsWorld::CreateDynamicBox(const Vector3& _pos, const Quaternion& _rot, const Vector3& _halfExtents, float _density)
	{
		if(!m_physics || !m_scene || !m_material)
			return nullptr;

		PxRigidDynamic* body = PxCreateDynamic(
			*m_physics, 
			PxTransform(ToPx(_pos), ToPx(_rot)),
			PxBoxGeometry(_halfExtents.x, _halfExtents.y, _halfExtents.z),
			*m_material,
			_density);

		if (body == nullptr)
			return nullptr;

		m_scene->addActor(*body);
		return body;
	}

	physx::PxRigidActor* PhysicsWorld::CreateStaticBox(const Vector3& _pos, const Quaternion& _rot, const Vector3& _halfExtents)
	{
		if (!m_physics || !m_scene || !m_material)
			return nullptr;

		PxRigidStatic* body = PxCreateStatic(
			*m_physics,
			PxTransform(ToPx(_pos), ToPx(_rot)),
			PxBoxGeometry(_halfExtents.x, _halfExtents.y, _halfExtents.z),
			*m_material);

		if (body == nullptr)
			return nullptr;

		m_scene->addActor(*body);
		return body;
	}

	physx::PxRigidActor* PhysicsWorld::CreateDynamicCapsule(const Vector3& _pos, const Quaternion& _rot, float _radius, float _height, float _density)
	{
		if (!m_physics || !m_scene || !m_material)
			return nullptr;

		PxRigidDynamic* body = PxCreateDynamic(
			*m_physics,
			PxTransform(ToPx(_pos), ToPx(_rot)),
			PxCapsuleGeometry(_radius, _height),
			*m_material,
			_density
		);

		if (body == nullptr)
			return nullptr;

		m_scene->addActor(*body);
		return body;
	}

	void PhysicsWorld::SetDefaultCollisionGroup()
	{
		// 가급적 런타임 중에 동적으로 이 부분을 제어할 일이 없어야 함
		// 그러므로 별도 인터페이스를 만들지 않을 것

		// 프로젝트 공통적으로 적용할 기본 충돌 레이어 설정
		/*
			Player - Player			O
			Player - Land			O
			Player - Obstacle		O

			Land - Land				O
			Land - Obstacle			X

			Obstacle - Obstacle		X
		*/

		// 기본적으로 0~31 레이어 모두 true
		// false만 명시
		PxSetGroupCollisionFlag(ECollisionGroup::Land,		ECollisionGroup::Obstacle,		false);
		PxSetGroupCollisionFlag(ECollisionGroup::Obstacle,	ECollisionGroup::Obstacle,		false);

		for (uint16_t i = 0; i < ECollisionGroup::END; ++i)
			PxSetGroupCollisionFlag(ECollisionGroup::IgnoreAll, i, false);
	}
}


