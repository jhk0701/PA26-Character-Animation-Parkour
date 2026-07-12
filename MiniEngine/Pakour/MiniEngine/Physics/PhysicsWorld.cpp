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
		inline Vector3 ToVec3(const physx::PxVec3& v) { return Vector3(v.x, v.y, v.z); }
	}

	void* RaycastResult::GetActor() const { return m_hitActor->userData; }

	void PhysicsWorld::SetQueryLayer(physx::PxRigidActor& _actor, uint32_t _layerMask)
	{
		// shape 수는 현재 팩토리가 만드는 액터 기준 항상 1, 
		// 복합 셰이프로 확장돼도 동작하도록 전부 순회
		const PxU32 count = _actor.getNbShapes();
		std::vector<PxShape*> shapes(count);
		_actor.getShapes(shapes.data(), count);

		const PxFilterData filterData(_layerMask, 0, 0, 0);
		for (PxShape* shape : shapes)
			shape->setQueryFilterData(filterData);
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

		m_controllerManager = PxCreateControllerManager(*m_scene);
		if (!m_controllerManager)
			MG_LOG_ERROR("PhysX : PxCreateControllerManager failed");

		SetDefaultCollisionGroup();

		return true;
	}

	void PhysicsWorld::Shutdown()
	{
		// 생성 역순 release. Material 은 m_physics 소유라 별도 release 안 함(참조만 해제).
		if (m_controllerManager)
		{
			m_controllerManager->release();
			m_controllerManager = nullptr;
		}

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

		// word0 = 0 인 shape는 마스크가 있는 모든 레이에 안맞을 것
		// 그러니 마스크를 활용하려거든 SetQueryLayer 설정
		SetQueryLayer(*body, ToMask(Layer::Default));
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

		SetQueryLayer(*body, ToMask(Layer::Default));
		m_scene->addActor(*body);
		return body;
	}

	void PhysicsWorld::ToggleDebugMode(bool _bIsOn)
	{
		if (!m_physics || !m_scene || !m_material)
			return;

		m_bIsDebugging = _bIsOn;

		m_scene->setVisualizationParameter(PxVisualizationParameter::eSCALE, m_bIsDebugging);
		m_scene->setVisualizationParameter(PxVisualizationParameter::eCOLLISION_SHAPES, m_bIsDebugging);
	}

	physx::PxController* PhysicsWorld::CreateCapsuleController(const CapsuleControllerDesc& _desc)
	{
		if (!m_controllerManager || !m_material)
			return nullptr;

		PxCapsuleControllerDesc d;
		d.radius = _desc.radius;
		d.height = _desc.height;
		d.stepOffset = _desc.stepOffset;
		d.contactOffset = _desc.contactOffset;
		d.slopeLimit = cosf(ToRadians(_desc.slopeLimitDeg)); // 코사인 각으로 변환
		d.upDirection = PxVec3(0.0f, 1.0f, 0.0f);
		d.material = m_material;
		d.position = PxExtendedVec3(
			static_cast<PxExtended>(_desc.footPosition.x),
			static_cast<PxExtended>(_desc.footPosition.y),
			static_cast<PxExtended>(_desc.footPosition.z)
		);

		if (!d.isValid())
		{
			MG_LOG_ERROR("PhysX : PxCapsuleController가 유효하지 않음\n(radius > 0, height > 0, stepOffset <= height + 2 * radius 확인 필요).");
			return nullptr;
		}

		PxController* cont = m_controllerManager->createController(d);
		if (!cont)
		{
			MG_LOG_ERROR("PhysX : Create Controller Failed");
			return nullptr;
		}

		// 캡슐 중심 위치 설정
		// 씬에 생성 직후라 텔레포트 관련해서 안전
		cont->setFootPosition(d.position);

		if (PxRigidDynamic* actor = cont->getActor())
			SetQueryLayer(*actor, ToMask(Layer::Default));

		return cont;
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

	bool PhysicsWorld::Raycast(const RaycastParam& _inParam, RaycastResult& _outResult, uint32_t _layerMask) const
	{
		// query filter 활용 _layerMask 에 비트가 켜진 레이어의 shape만 탐지 
		// ToMask(Layer::Character) → 캐릭터
		// Layer::Ground|Layer::Prop → 지형과 소품만

		if (!m_scene || _inParam.m_maxDistance <= 0.0f)
			return false;

		// 필터없음은 모두 통과될 것이므로 애초에 쏘지 않을 것
		if (_layerMask == LayerMask::NONE)
			return false;

		// 쿼리 필터 생성
		// word0 자리에 layerMask
		const PxQueryFilterData filter(
			PxFilterData(_layerMask, 0, 0, 0),
			PxQueryFlag::eSTATIC | PxQueryFlag::eDYNAMIC
		);

		PxRaycastBuffer hit;
		bool bIsHit = m_scene->raycast(
			ToPx(_inParam.m_origin), 
			ToPx(_inParam.m_dir), 
			physx::PxReal(_inParam.m_maxDistance),
			hit,
			PxHitFlag::eDEFAULT,
			filter) 
			&& hit.hasBlock;

		if (bIsHit) 
		{
			const PxRaycastHit& block = hit.block;
			_outResult.m_pos = ToVec3(block.position);
			_outResult.m_nrm = ToVec3(block.normal);
			_outResult.m_distance = block.distance;
			_outResult.m_hitActor = block.actor;
			_outResult.m_hitShape = block.shape;
		}

		return bIsHit;
	}
}
