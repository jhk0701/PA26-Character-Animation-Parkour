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

	void* RaycastResult::GetActor() const 
	{ 
		return m_hitActor->userData;
	}

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

	void PhysicsWorld::SetCollisionLayer(physx::PxRigidActor& _actor, uint32_t _layerMask)
	{
		const PxU32 count = _actor.getNbShapes();
		std::vector<PxShape*> shapes(count);
		_actor.getShapes(shapes.data(), count);

		const PxFilterData filterData(_layerMask, 0, 0, 0);
		for (PxShape* shape : shapes)
			shape->setSimulationFilterData(filterData);
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

	bool PhysicsWorld::Raycast(const RaycastParam& _inParam, RaycastResult& _outResult, uint32_t _layerMask) const
	{
		// query filter 활용 _layerMask 에 비트가 켜진 레이어의 shape만 탐지 
		// ToMask(Layer::Character) → 캐릭터
		// Layer::Ground | Layer::Prop → 지형과 소품만
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

		PxRaycastBuffer hitBuffer;
		bool bIsHit = m_scene->raycast(
			ToPx(_inParam.m_origin), 
			ToPx(_inParam.m_dir), 
			physx::PxReal(_inParam.m_maxDistance),
			hitBuffer,
			PxHitFlag::eDEFAULT,
			filter) 
			&& hitBuffer.hasBlock;

		_outResult.m_bIsHit = bIsHit;
		if (bIsHit) 
		{
			const PxRaycastHit& block = hitBuffer.block;
			_outResult.m_pos = ToVec3(block.position);
			_outResult.m_nrm = ToVec3(block.normal);
			_outResult.m_distance = block.distance;
			_outResult.m_hitActor = block.actor;
			_outResult.m_hitShape = block.shape;
		}

		RecordQueryLine(_inParam.m_origin, _inParam.m_dir, _inParam.m_maxDistance, bIsHit, _outResult);
		return bIsHit;
	}

	bool PhysicsWorld::CapsuleCast(const CapsulecastParam& _inParam, RaycastResult& _outResult, uint32_t _layerMask/* = LayerMask::ALL*/) const
	{
		if (!m_scene || _inParam.m_maxDistance <= 0.0f)
			return false;

		// 필터없음은 모두 통과될 것이므로 애초에 쏘지 않을 것
		if (_layerMask == LayerMask::NONE)
			return false;

		// 레이캐스트와 동일하게 마스크 설정
		const PxQueryFilterData filter(
			PxFilterData(_layerMask, 0, 0, 0),
			PxQueryFlag::eSTATIC | PxQueryFlag::eDYNAMIC
		);
		PxCapsuleGeometry capsule(_inParam.m_radius, _inParam.m_halfHeight);
		PxTransform pose(ToPx(_inParam.m_startPos), ToPx(_inParam.m_startRot));

		PxSweepBuffer hitBuffer;
		bool bIsHit = m_scene->sweep(
			capsule, pose,
			ToPx(_inParam.m_dir),
			physx::PxReal(_inParam.m_maxDistance),
			hitBuffer,
			PxHitFlag::eDEFAULT,
			filter)
			&& hitBuffer.hasBlock;

		_outResult.m_bIsHit = bIsHit;
		if (bIsHit) 
		{
			const PxSweepHit& block = hitBuffer.block;
			_outResult.m_pos = ToVec3(block.position);
			_outResult.m_nrm = ToVec3(block.normal);
			_outResult.m_distance = block.distance;
			_outResult.m_hitActor = block.actor;
			_outResult.m_hitShape = block.shape;
		}

		RecordQueryLine(_inParam.m_startPos, _inParam.m_dir, _inParam.m_maxDistance, bIsHit, _outResult);
		return bIsHit;
	}
	

	namespace
	{
		// 히트 지점에서 표면 법선을 얼마나 길게 그릴지(월드 단위).
		constexpr float QUERY_NORMAL_LENGTH = 0.25f;
		// 드레인 누락 시 무한 증가 방지 상한. 초과분은 조용히 버린다.
		constexpr size_t MAX_QUERY_LINES = 4096;
	}


	void PhysicsWorld::SetDebugVisualization(bool _enable, float _scale)
	{
		if (!m_scene)
			return;

		m_scene->setVisualizationParameter(PxVisualizationParameter::eSCALE, _enable ? 1.0f : 0.0f);
		m_scene->setVisualizationParameter(PxVisualizationParameter::eCOLLISION_SHAPES, _scale);
		// m_scene->setVisualizationParameter(PxVisualizationParameter::eACTOR_AXES, _scale);
	}

	void PhysicsWorld::SetDrawQueries(bool _enable)
	{
		m_drawQueries = _enable;
		if (!_enable)
			m_queryLines.clear();
	}

	void PhysicsWorld::CollectDebugLines(std::vector<DebugLine>& _out)
	{
		if (!m_scene)
			return;

		const PxRenderBuffer& buffer = m_scene->getRenderBuffer();

		const PxU32 lineCnt = buffer.getNbLines();
		const PxDebugLine* lines = buffer.getLines();

		_out.reserve(_out.size() + lineCnt + m_queryLines.size());

		for (PxU32 i = 0; i < lineCnt; ++i)
		{
			const PxDebugLine& line = lines[i];
			_out.push_back({ 
				Vector3(line.pos0.x, line.pos0.y, line.pos0.z), 
				Vector3(line.pos1.x, line.pos1.y, line.pos1.z), 
				line.color0 
				});
		}

		_out.insert(_out.end(), m_queryLines.begin(), m_queryLines.end());
		m_queryLines.clear();
	}

	void PhysicsWorld::RecordQueryLine(const Vector3& _origin, const Vector3& _unitDir, float _maxDistance,
		bool _hit, const RaycastResult& _hitInfo) const
	{
		if (!m_drawQueries || m_queryLines.size() + 3 > MAX_QUERY_LINES)
			return;

		const Vector3 end = _origin + _unitDir * _maxDistance;
		if (!_hit)
		{
			m_queryLines.push_back({ _origin, end, DebugColor::RED }); // 미스: 전 구간 빨강
			return;
		}

		// 히트: origin→히트점 초록, 히트점→끝 빨강(레이가 어디까지 갈 예정이었는지), 법선 파랑.
		m_queryLines.push_back({ _origin, _hitInfo.m_pos, DebugColor::GREEN });
		m_queryLines.push_back({ _hitInfo.m_pos, end, DebugColor::RED });
		m_queryLines.push_back({ _hitInfo.m_pos,
								 _hitInfo.m_pos + _hitInfo.m_nrm * QUERY_NORMAL_LENGTH,
								 DebugColor::BLUE });
	}
}
