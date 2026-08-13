#include "pch.h"
#include "Character.h"

#include "Core/Math.h"
#include "Core/Log.h"
#include "Core/DebugMarkers.h"

#include "Physics/CollsionLayer.h"
#include "Platform/Input.h"
#include "Manager/AssetManager.h"
#include "Manager/DataManager.h"
#include "Manager/PathManager.h"

#include "Scene/Scene.h"
#include "Scene/RigidBodyComponent.h"
#include "Scene/SkeletalMeshComponent.h"
#include "Scene/CharacterControllerComponent.h"
#include "Animation/Animator.h"
#include "Perception/PerceptionComponent.h"
#include "Perception/ProcessorComponent.h"

#include "Content/ContentConfig.h"
#include "Content/Data/CharacterConfigData.h"
#include "Content/Data/CharacterAnimData.h"
#include "Content/Data/ActionClipContainer.h"
#include "Content/Data/PerceptionQueryTree.h"
#include "Content/Data/ProcessConditionData.h"

#include "Content/CharacterStateMachine.h"
#include "Content/CharacterState/LandingState.h"
#include "Content/CharacterState/InAirState.h"
#include "Content/CharacterState/HangingState.h"
#include "Content/CharacterState/BeamState.h"

using namespace Content::Config;

namespace
{
	constexpr const char* STATE_NAMES[] =
	{
		"Landing",
		"InAir",
		"Hanging",
		"BeamStand",
		"BeamHanging",
		"PoleHanging",
	};

	// 누락 체크용 매크로
	static_assert(std::size(STATE_NAMES) == static_cast<size_t>(Character::EState::End), 
		"STATE_NAMES 가 Character::EState 와 다름");
}

bool Character::TryParseState(const std::string& _name, uint8_t& _outState)
{
	for (size_t i = 0; i < std::size(STATE_NAMES); ++i)
	{
		if (_name == STATE_NAMES[i])
		{
			_outState = static_cast<uint8_t>(i);
			return true;
		}
	}

	return false;
}

const char* Character::GetStateName(uint8_t _state)
{
	if (_state >= std::size(STATE_NAMES))
		return "invalid";

	return STATE_NAMES[_state];
}


Character::Character() { }
Character::~Character() { }

void Character::Construct(const Vector3& _initPosition, const std::wstring& _charPath)
{
	std::shared_ptr<SceneComponent> pRoot = AddComponent<SceneComponent>();
	m_skinMeshComp = AddComponent<SkeletalMeshComponent>();
	PathManager* pathMgr = PathManager::GetInstance();

	std::wstring miniPath = pathMgr->ResolveAssetPath(_charPath.c_str());
	std::shared_ptr<SkinnedMesh> skinnedMesh = AssetManager::GetInstance()->LoadSkinnedMesh(miniPath);

	std::shared_ptr<SkeletalMeshComponent> skinComp = GetSkin().lock();
	skinComp->SetMesh(skinnedMesh);
	skinComp->AttachTo(GetRoot());

	InitAnimation(skinComp);

	{
		std::shared_ptr<LimbIKComponent> pLimbIK = AddComponent<LimbIKComponent>();
		LimbIKDesc desc;
		desc.footHeight = 0.1f;
		desc.maxFootDrop = 0.4f;
		desc.maxFootRaise = 0.4f;
		desc.maxPelvisDrop = 0.45f;
		desc.alphaFadeSpeed = 20.0f;
		desc.poleDir[(uint8_t)ELimbType::LeftArm] = Vector3(-1.0f, -0.0f, -1.0f);
		desc.poleDir[(uint8_t)ELimbType::RightArm] = Vector3(1.0f, -0.0f, -1.0f);
		desc.poleDir[(uint8_t)ELimbType::LeftLeg] = Vector3(-0.2f, 0.5f, 1.0f);
		desc.poleDir[(uint8_t)ELimbType::RightLeg] = Vector3(0.2f, 0.5f, 1.0f);

		// 관절 한계 degree 
		// 굽힘각은 lower 관절 내각 
		// 0 = 완전히 접힘, 180 = 완전히 펴짐
		desc.maxBendDeg[(uint8_t)ELimbType::LeftArm] = 165.0f;
		desc.maxBendDeg[(uint8_t)ELimbType::RightArm] = 165.0f;
		desc.maxBendDeg[(uint8_t)ELimbType::LeftLeg] = 165.0f;
		desc.maxBendDeg[(uint8_t)ELimbType::RightLeg] = 165.0f;

		desc.minBendDeg[(uint8_t)ELimbType::LeftArm] = 20.0f;
		desc.minBendDeg[(uint8_t)ELimbType::RightArm] = 20.0f;
		desc.minBendDeg[(uint8_t)ELimbType::LeftLeg] = 25.0f;
		desc.minBendDeg[(uint8_t)ELimbType::RightLeg] = 25.0f;

		pLimbIK->Init(skinComp, desc);
		m_limbIKComp = pLimbIK;
	}
	{
		// 캐릭터 컨트롤러 설정
		std::shared_ptr<CharacterControllerComponent> pCharCont = AddComponent<CharacterControllerComponent>();
		Physics::CapsuleControllerDesc desc;
		desc.radius = m_capsuleRadius;
		desc.height = m_capsuleHeight;
		desc.stepOffset = m_stepOffset;
		desc.contactOffset = m_capsuleContactOffset;
		pCharCont->Init(*GetScene()->GetPhysics().lock(), desc, GetRoot());

		pCharCont->SetRootMotionSource(m_skinMeshComp.lock());
		pCharCont->SetQueryLayer(MiniEngine::Physics::Layer::Character);
		pCharCont->SetFallingSecondThreshold(0.25f); // 낙하 인정 시간 설정

		m_charCont = pCharCont;
	}
	{
		// 지형 인식 및 처리용 
		m_perception = AddComponent<PerceptionComponent>();
		m_processor = AddComponent<ProcessorComponent>();
	}
	{
		// 캐릭터 로직 처리용 FSM 추가
		std::shared_ptr<CharacterStateMachine> pCharFSM = AddComponent<CharacterStateMachine>();
		// enum class EState : uint8_t 순서대로 사용중
		pCharFSM->RegisterStates(
			{
				std::make_shared<LandingState>(),
				std::make_shared<InAirState>(),
				std::make_shared<HangingState>(),
				std::make_shared<BeamStandState>(),
				std::make_shared<BeamHangingState>(),
				std::make_shared<PoleHangingState>()
			});
		m_charFSM = pCharFSM;
	}
	
	pRoot->localTransform.position = _initPosition;
	PostConstruct();
}

void Character::PostConstruct()
{
	// 컴포넌트들 초기화 이후 호출
	// 컴포넌트 간 연결 사항 정리
	std::shared_ptr<Animator> pAnim = GetAnim().lock();
	pAnim->SetOverrideTrackStartEvent([this]() { GetController().lock()->SetCheckFalling(false); });
	pAnim->SetOverrideTrackEndEvent([this]() { GetController().lock()->SetCheckFalling(true); });

	m_charFSM.lock()->Transition((uint8_t)EState::Landing);
}

void Character::OnBeforeSortComponent()
{
	Pawn::OnBeforeSortComponent();

	// 컴포넌트 우선순위 설정
	// m_charFSM.lock()->SetSortOrder(0);				단순 명시용, FSM에 따라 인지 결과 처리
	// m_charCont.lock()->SetSortOrder(0);				// 1. 게임 로직 입력에 따라 움직인다.
	m_limbIKComp.lock()->SetSortOrder(1);				// 2. 움직인 후 IK 타겟을 설정한다.
	m_skinMeshComp.lock()->SetSortOrder(2);				// 3. IK Solver 호출로 필요한 지점에 타켓 위치시킨다.

	// 이후 Rendering될 것
}

void Character::BeginPlay()
{
	Pawn::BeginPlay();
	
	LoadData();
	
	InitCollisionLayer();
	m_charFSM.lock()->Start();
	m_charCont.lock()->SetCheckFalling(true);
}

void Character::LoadData()
{
	std::shared_ptr<CharacterConfigData> pConfig;
	if (DataManager::GetInstance()->TryGetDataAsset<CharacterConfigData>(L"CharacterConfig.json", pConfig) == false)
		MG_LOG_ERROR("[Character::LoadData] CharacterConfig.json not found.");

	m_pConfig = pConfig;

	// 지형 인식 트리
	std::shared_ptr<PerceptionQueryData> pQueryData;
	if (DataManager::GetInstance()->TryGetDataAsset<PerceptionQueryData>(L"PerceptionQueryData.json", pQueryData) == false || 
		pQueryData->IsValid() == false)
	{
		MG_LOG_ERROR("[Character::LoadData] PerceptionQueryData.json load failed. perception disabled.");
		return;
	}

	std::shared_ptr<PerceptionNode> pQueryTree = pQueryData->ConstructTree();
	if (pQueryTree == nullptr)
	{
		MG_LOG_ERROR("[Character::LoadData] failed to construct query tree. perception disabled.");
		return;
	}

	m_perception.lock()->SetQueryTree(std::move(pQueryTree));

	std::vector<std::shared_ptr<ProcessCondition>> conditions;
	std::vector<std::shared_ptr<ProcessData>> processDatas;
	std::shared_ptr<ProcessConditionData> pProcessCondition;
	if (DataManager::GetInstance()->TryGetDataAsset<ProcessConditionData>(L"ProcessConditionData.json", pProcessCondition) == false ||
		pProcessCondition->IsValid() == false) 
	{
		MG_LOG_ERROR("[Character::LoadData] ProcessConditionData.json load failed. perception disabled.");
		return;
	}
	pProcessCondition->ConstructData(conditions, processDatas);

	// MG_LOG_INFO("[Character::LoadData] {} processData is loaded.", processDatas.size());

	m_processor.lock()->Init(std::move(conditions), std::move(processDatas));
}

bool Character::TryPerception()
{
	return TryPerception(GetRoot()->localTransform.Forward());
}

bool Character::TryPerception(const Vector3& _dir)
{
	if (GetAnim().lock()->IsActionClipPlaying((uint8_t)EActionPriority::Override))
		return false; // 이미 행동 중이라면 탐색하지 않도록

	std::shared_ptr<PerceptionComponent> pPercept = m_perception.lock();
	EPerceptionResult perceptResult = pPercept->Travel(_dir); // 탐색 개시

	if (perceptResult != EPerceptionResult::Succeess)
	{
		// 빈 결과는 리턴
		MG_LOG_INFO("[Character::TryPerception] Perception Travel Result is not success");
		return false;
	}

	ProcessPerceptionResult(pPercept->GetResult()); // 탐색 결과 확인

	return true;
}

void Character::ProcessPerceptionResult(const TravelResult& _result)
{
	if (_result.m_pFirstObstacle)
	{
		m_curObstacleInfo.m_bIsNewObstacle = m_curObstacleInfo.m_pObstacle != _result.m_pFirstObstacle;
		m_curObstacleInfo.m_pObstacle = _result.m_pFirstObstacle;

		m_curObstacleInfo.m_obstacleHitPos = _result.m_firstObstacleHitPos;
		m_curObstacleInfo.m_obstacleHitNrm = _result.m_firstObstacleHitNrm;
		m_curObstacleInfo.m_obstacleDistance = _result.m_obstacleDistance;

		m_curObstacleInfo.m_obstacleLedge = _result.m_obstacleLedge;
		m_curObstacleInfo.m_bDetectLedge = _result.m_bDetectLedge;

		m_curObstacleInfo.m_obstacleDepth = _result.m_obstacleDepth;
		m_curObstacleInfo.m_obstacleHeight = _result.m_obstacleLedge - GetRoot()->localTransform.position.y;
	}
	else
	{
		m_curObstacleInfo.m_pObstacle = nullptr;
		MG_LOG_WARN("[Character] Travel Result returned but Cur Obstacle is null");
	}

	uint8_t processResult = 0;
	if (m_processor.lock()->ProcessResult(_result, processResult) == false)
	{
		MG_LOG_WARN("[Character::ProcessPerceptionResult] Result try to process, but no matched result exists :: {}", processResult);
		return;
	}

	// 액션 수행
	if (std::shared_ptr<ActionClip> pAction = GetActions(processResult))
		PlayActionClip(pAction, 0.2f, (uint8_t)EActionPriority::Override);
}

void Character::InitCollisionLayer()
{
	m_charCont.lock()->SetLayerCollisionEnabled(MiniEngine::Physics::Layer::ObstacleLedge, false);
}
void Character::SetEnableCollisionObstacle(bool _bEnable)
{
	m_charCont.lock()->SetLayerCollisionEnabled(MiniEngine::Physics::Layer::Obstacle, _bEnable);
}

void Character::ReserveIKDetectGround()
{
	if (m_limbIKComp.expired())
		return;

	std::shared_ptr<LimbIKComponent> pLimbIK = m_limbIKComp.lock();
	pLimbIK->SetPendingTask(ELimbType::LeftLeg, [this]() { return IKDetectGround((uint8_t)ELimbType::LeftLeg); });
	pLimbIK->SetPendingTask(ELimbType::RightLeg, [this]() { return IKDetectGround((uint8_t)ELimbType::RightLeg); });
}

LimbIKComponent::TaskResult Character::IKDetectGround(uint8_t _ik)
{
	std::shared_ptr<LimbIKComponent> pIKComp = m_limbIKComp.lock();
	std::shared_ptr<SkeletalMeshComponent> pSkin = m_skinMeshComp.lock();
	std::shared_ptr<Animator> pAnim = pSkin->GetAnim().lock();

	LimbIKComponent::TaskResult result;
	result.posAlpha = 0.0f;
	result.rotAlpha = 0.0f;

	// 예약된 작업은 override 사용, 이동 중엔 무시될 것
	if (pAnim->IsActionClipPlaying() ||
		m_inputDir.LengthSquared() > 1e-4f)
		return result;

	const int BONE_IDX = pSkin->GetMesh().lock()->GetHumanoidBones().Get(pIKComp->GetBinding((ELimbType)_ik).end);
	if (BONE_IDX < 0)
		return result;

	Matrix endBoneW;
	if (!pSkin->GetBoneWorldMatrix(BONE_IDX, endBoneW))
		return result;

	result.position = endBoneW.Translation();

	Physics::RaycastParam param;
	param.m_dir = Vector3(0.0f, -1.0f, 0.0f);
	param.m_maxDistance = m_ikRayDistance * 2.0f;
	param.m_origin = result.position;
	param.m_origin.y += m_ikRayDistance;

	Physics::RaycastResult hitResult;

	std::shared_ptr<Physics::PhysicsWorld> pPhysics = GetScene()->GetPhysics().lock();
	if (!pPhysics->Raycast(param, hitResult, Physics::Layer::Ground | Physics::Layer::Obstacle))
		return result;
	
	// MiniEngine::Debug::DrawPoint(hitResult.m_pos, MiniEngine::DebugColor::YELLOW, 0.05f, MiniEngine::Debug::EMarkerShape::Sphere, 0.01f);

	// 위치 적용
	pIKComp->SetOriginPosIK((ELimbType)_ik, result.position);
	result.position = hitResult.m_pos;
	result.posAlpha = 1.0f;

	// 회전 적용
	hitResult.m_nrm.Normalize();
	const Vector3 UP(0.0f, 1.0f, 0.0f);
	const float SLOPE = std::acos(std::clamp(UP.Dot(hitResult.m_nrm), -1.0f, 1.0f)); // 경사각 라디안
	Quaternion q = FromToRotation(UP, hitResult.m_nrm);

	const float MAX_SLOPE = ToRadians(m_maxSlopeDeg);
	if (SLOPE > MAX_SLOPE && SLOPE > 1e-4f)
		result.rotation = Quaternion::Slerp(Quaternion(0.0f, 0.0f, 0.0f, 1.0f), q, MAX_SLOPE / SLOPE);
	else
		result.rotation = q;

	result.rotation.Normalize();
	result.rotAlpha = 1.0f;
	return result;
}

void Character::ReserveIKDetectWall()
{
	if (m_limbIKComp.expired())
		return;

	std::shared_ptr<LimbIKComponent> pLimbIK = m_limbIKComp.lock();
	pLimbIK->SetPendingTask(ELimbType::LeftArm, [this]() { return IKDetectWall((uint8_t)ELimbType::LeftArm); });
	pLimbIK->SetPendingTask(ELimbType::RightArm, [this]() { return IKDetectWall((uint8_t)ELimbType::RightArm); });
	pLimbIK->SetPendingTask(ELimbType::LeftLeg, [this]() { return IKDetectWall((uint8_t)ELimbType::LeftLeg); });
	pLimbIK->SetPendingTask(ELimbType::RightLeg, [this]() { return IKDetectWall((uint8_t)ELimbType::RightLeg); });
}

LimbIKComponent::TaskResult Character::IKDetectWall(uint8_t _ik)
{
	std::shared_ptr<LimbIKComponent> pIKComp = m_limbIKComp.lock();
	std::shared_ptr<SkeletalMeshComponent> pSkin = m_skinMeshComp.lock();
	std::shared_ptr<Animator> pAnim = pSkin->GetAnim().lock();

	LimbIKComponent::TaskResult result;
	result.posAlpha = 0.0f;
	result.rotAlpha = 0.0f;

	if (pAnim->IsActionClipPlaying())
		return result;

	const int BONE_IDX = pSkin->GetMesh().lock()->GetHumanoidBones().Get(pIKComp->GetBinding((ELimbType)_ik).end);
	if (BONE_IDX < 0)
		return result;

	Matrix endBoneW;
	if (!pSkin->GetBoneWorldMatrix(BONE_IDX, endBoneW))
		return result;

	result.position = endBoneW.Translation();

	// 1. 현재 뼈의 end 위치에서 캐릭터의 후면으로 0.05 이동
	// 2. 캐릭터의 정면으로 0.1 방향으로 레이캐스트
	// 3. 접촉면이 손, 발의 위치
	const Transform& TF = GetRoot()->localTransform;

	Physics::SpherecastParam param;
	param.m_dir = TF.Forward();
	param.m_maxDistance = m_ikRayDistance * 2.0f;
	param.m_startPos = result.position + param.m_dir * -m_ikRayDistance;
	param.m_radius = 0.1f;

	Physics::RaycastResult hitResult;
	std::shared_ptr<Physics::PhysicsWorld> pPhysics = GetScene()->GetPhysics().lock();
	if (!pPhysics->SphereCast(param, hitResult, Physics::ToMask(Physics::Layer::Obstacle)))
		return result;

	// 벽으로부터 일정 범위 이내에 있는 경우는 ik를 적용하지 않음
	// MiniEngine::Debug::DrawPoint(hitResult.m_pos, MiniEngine::DebugColor::YELLOW, 0.05f, MiniEngine::Debug::EMarkerShape::Sphere, 0.01f);

	// 위치 적용
	pIKComp->SetOriginPosIK((ELimbType)_ik, result.position);		
	result.position = hitResult.m_pos - TF.Forward() * m_ikWallOffset[(ELimbType)_ik];
	result.posAlpha = 1.0f;

	// hitResult.m_nrm.Normalize();
	// result.rotation.Normalize();
	// result.rotAlpha = 1.0;
	return result;
}

void Character::ReserveIKDetectBeamHanging()
{
	if (m_limbIKComp.expired())
		return;

	std::shared_ptr<LimbIKComponent> pLimbIK = m_limbIKComp.lock();
	pLimbIK->SetPendingTask(ELimbType::LeftArm, [this]() { return IKDetectBeamHanging((uint8_t)ELimbType::LeftArm); });
	pLimbIK->SetPendingTask(ELimbType::RightArm, [this]() { return IKDetectBeamHanging((uint8_t)ELimbType::RightArm); });
}

LimbIKComponent::TaskResult Character::IKDetectBeamHanging(uint8_t _ik)
{
	LimbIKComponent::TaskResult result;
	result.posAlpha = 0.0f;
	result.rotAlpha = 0.0f;

	std::shared_ptr<LimbIKComponent> pIKComp = m_limbIKComp.lock();
	std::shared_ptr<SkeletalMeshComponent> pSkin = m_skinMeshComp.lock();
	std::shared_ptr<Animator> pAnim = pSkin->GetAnim().lock();

	if (pAnim->IsActionClipPlaying())
		return result;

	const int BONE_IDX = pSkin->GetMesh().lock()->GetHumanoidBones().Get(pIKComp->GetBinding((ELimbType)_ik).end);
	if (BONE_IDX < 0)
		return result;

	Matrix endBoneW;
	if (!pSkin->GetBoneWorldMatrix(BONE_IDX, endBoneW))
		return result;

	result.position = endBoneW.Translation();

	const Transform& TF = GetRoot()->localTransform;

	Physics::SpherecastParam param;
	param.m_dir = TF.Up();
	param.m_maxDistance = m_ikRayDistance * 2.0f;
	param.m_startPos = result.position + param.m_dir * -m_ikRayDistance;
	param.m_radius = 0.1f;

	Physics::RaycastResult hitResult;
	std::shared_ptr<Physics::PhysicsWorld> pPhysics = GetScene()->GetPhysics().lock();
	if (!pPhysics->SphereCast(param, hitResult, Physics::ToMask(Physics::Layer::ObstacleLedge)))
		return result;

	// 위치 적용
	pIKComp->SetOriginPosIK((ELimbType)_ik, result.position);
	result.position = hitResult.m_pos;
	result.posAlpha = 1.0f;
	
	// MiniEngine::Debug::DrawPoint(hitResult.m_pos, MiniEngine::DebugColor::YELLOW, 0.05f, MiniEngine::Debug::EMarkerShape::Sphere, 0.01f);
	return result;
}

void Character::ReserveIKDetectPole()
{
	if (m_limbIKComp.expired())
		return;

	std::shared_ptr<LimbIKComponent> pLimbIK = m_limbIKComp.lock();
	pLimbIK->SetPendingTask(ELimbType::LeftArm, [this]() { return IKDetectPole((uint8_t)ELimbType::LeftArm); });
	pLimbIK->SetPendingTask(ELimbType::RightArm, [this]() { return IKDetectPole((uint8_t)ELimbType::RightArm); });
	pLimbIK->SetPendingTask(ELimbType::LeftLeg, [this]() { return IKDetectPole((uint8_t)ELimbType::LeftLeg); });
	pLimbIK->SetPendingTask(ELimbType::RightLeg, [this]() { return IKDetectPole((uint8_t)ELimbType::RightLeg); });
}

LimbIKComponent::TaskResult Character::IKDetectPole(uint8_t _ik)
{
	LimbIKComponent::TaskResult result;
	result.posAlpha = 0.0f;
	result.rotAlpha = 0.0f;

	std::shared_ptr<SkeletalMeshComponent> pSkin = m_skinMeshComp.lock();
	if (pSkin->GetAnim().lock()->IsActionClipPlaying())
		return result;

	IObstacle* pObs = GetCurObstacle();
	if (pObs == nullptr || m_limbIKComp.expired())
		return result;

	std::shared_ptr<LimbIKComponent> pIKComp = m_limbIKComp.lock();

	// ik 손, 발 위치 고정
	// y축 제외, x,z에 대해서 고정
	const int BONE_IDX = pSkin->GetMesh().lock()->GetHumanoidBones().Get(pIKComp->GetBinding((ELimbType)_ik).end);
	if (BONE_IDX < 0)
		return result;

	Matrix endBoneW;
	if (!pSkin->GetBoneWorldMatrix(BONE_IDX, endBoneW))
		return result;

	Vector3 originalPos = endBoneW.Translation();
	pIKComp->SetOriginPosIK((ELimbType)_ik, originalPos);

	Vector3 polePos = pObs->GetTransform().position;
	result.position = 
		Vector3(polePos.x, originalPos.y, polePos.z) + 
		GetRoot()->localTransform.Right() * m_ikPoleOffset[(ELimbType)_ik];
	result.posAlpha = 1.0f;

	return result;
}

void Character::ClearIKReserve()
{
	m_limbIKComp.lock()->ClearPendingTask();
}

void Character::SetIKPoleVector(uint8_t _ik, const Vector3& _newVec)
{
	m_limbIKComp.lock()->UpdatePoleVector((ELimbType)_ik, _newVec);
}

void Character::IKDetectObstacle(uint8_t _ik, const Vector3& _posOffset)
{
	// 파쿠르 중 장애물에 손, 발을 가져다 대는 용도
	// 이미 장애물의 위치 등을 알고 있기 때문에 레이캐스트를 쏘진 않을 것
	if (!m_curObstacleInfo.IsValid())
		return;

	Vector3 targetPos = m_curObstacleInfo.m_obstacleHitPos;
	targetPos.y = m_curObstacleInfo.m_obstacleLedge;
	targetPos += _posOffset;

	// MiniEngine::Debug::DrawPoint(targetPos, MiniEngine::DebugColor::YELLOW, 0.05f, MiniEngine::Debug::EMarkerShape::Sphere, 0.01f);

	m_limbIKComp.lock()->SetTargetPosIK((ELimbType)_ik, targetPos);
}

void Character::IKSetFixedPoint(uint8_t _ik, const Vector3& _posOffset)
{
	if (!m_curObstacleInfo.IsValid())
		return;

	Vector3 targetPos = m_curObstacleInfo.m_obstacleHitPos;
	targetPos.y = GetRoot()->localTransform.position.y + GetCapsuleHalfHeight(); // contact offset은 무시
	targetPos += _posOffset;

	MiniEngine::Debug::DrawPoint(targetPos, MiniEngine::DebugColor::YELLOW, 0.05f, MiniEngine::Debug::EMarkerShape::Sphere, 0.01f);

	m_limbIKComp.lock()->SetTargetPosIK((ELimbType)_ik, targetPos);
}

void Character::SetIKAlpha(uint8_t _ik, float _alpha)
{
	m_limbIKComp.lock()->SetAlphaIK((ELimbType)_ik, _alpha);
}

void Character::AddMovementInput(const Vector3& _moveDelta)
{
	m_charCont.lock()->AddMovementInput(_moveDelta);
}
void Character::SetPosition(const Vector3& _newPos)
{
	m_charCont.lock()->SetPosition(_newPos);
}
void Character::ClearMovement()
{
	m_charCont.lock()->ClearMovement();
}
bool Character::IsFalling() const
{
	return m_charCont.lock()->IsFalling();
}
bool Character::IsGrounded() const
{
	return  m_charCont.lock()->IsGrounded();
}
float Character::GetVelocity() const
{
	return m_charCont.lock()->GetVelocity();
}
void Character::SetUseGravity(bool _bUse)
{
	std::shared_ptr<CharacterControllerComponent> pCharCont = m_charCont.lock();
	pCharCont->SetUseGravity(_bUse);
	pCharCont->SetForceFalling(false);
}

void Character::Jump()
{
	m_charCont.lock()->Jump(m_jumpSpeed);
}

void Character::InputJump()
{
	if (m_state == EState::InAir)
		return;
	
	uint8_t act = 0;
	if (m_state != EState::Landing)
		act = (uint8_t)ETagAct::JumpFromWall;
	else  
		act = (uint8_t)(GetInputDir().y > 0 ? ETagAct::JumpFront : ETagAct::Jump);

	if (std::shared_ptr<ActionClip> pJump = GetActions(act))
		GetAnim().lock()->PlayActionClip(pJump, 0.2f);
}

std::weak_ptr<Animator> Character::GetAnim() const
{
	return m_skinMeshComp.lock()->GetAnim();
}
void Character::SetAnimBaseTrackInputAxis(const Vector2& _input)
{
	GetAnim().lock()->SetBaseTrackInputAxis(_input);
}
void Character::TranstionBaseTrack(uint8_t _state, float _transitionTime)
{
	GetAnim().lock()->TranstionBaseTrack(_state, _transitionTime);
}
bool Character::IsActionClipPlaying() const
{
	return GetAnim().lock()->IsActionClipPlaying();
}
void Character::PlayActionClip(std::shared_ptr<ActionClip> _clip, float _transitionTime, uint8_t _priority)
{
	if (_clip == nullptr)
		return;

	GetAnim().lock()->PlayActionClip(_clip, _transitionTime, _priority);
}

const PerceptionConfig& Character::GetPerceptionConfig() const
{
	if (m_pConfig.expired())
	{
		static const CharacterConfig FALLBACK;
		static bool bWarned = false;

		if (bWarned == false)
		{
			bWarned = true;
			MG_LOG_ERROR("[Character::GetPerceptionConfig] config not loaded. using default values.");
		}

		return FALLBACK;
	}

	return m_pConfig.lock()->Config;
}

const CharacterConfig& Character::GetConfig() const
{
	if (m_pConfig.expired())
	{
		static const CharacterConfig FALLBACK;
		static bool bWarned = false;

		if (bWarned == false)
		{
			bWarned = true;
			MG_LOG_ERROR("[Character::GetConfig] config not loaded. using default values.");
		}

		return FALLBACK;
	}

	return m_pConfig.lock()->Config;
}

IObstacle* Character::GetCurObstacle() const
{
	return m_charFSM.lock()->GetCurrentObstacle();
}

void Character::TransitionStateMachine(uint8_t _state)
{
	SetState(static_cast<Character::EState>(_state));
	m_charFSM.lock()->Transition(_state);
}

void Character::InitAnimation(std::shared_ptr<SkeletalMeshComponent>& _skinComp)
{
	// 애니메이션 설정
	std::shared_ptr<Animator> pAnim = _skinComp->GetAnim().lock();
	pAnim->ReserveBaseLocomotion(static_cast<uint8_t>(EState::End));

	// 애니메이션 데이터 로드
	std::shared_ptr<CharacterAnimData> pAnimData;
	if (DataManager::GetInstance()->TryGetDataAsset<CharacterAnimData>(L"CharacterActionClips.json", pAnimData) == false)
		MG_LOG_ERROR("[Character::InitAnimation] CharacterActionClips.json not found.");

	ActionClipLoadParam param;
	param.pAnim = pAnim;
	param.pSources = _skinComp->GetMesh().lock();
	param.pMaps = &m_mapActions;
	param.pAnimData = pAnimData.get();
	ActionClipContainer::LoadActionClips(param);

	pAnim->SetEnableRootMotion(true);

	RootMotionConfig rmCfg;
	rmCfg.extractY = true;
	rmCfg.applyY = true;
	rmCfg.extractYaw = false;
	rmCfg.applyYaw = false;

	pAnim->SetRootMotionConfig(rmCfg);
	pAnim->SetRootBoneIdx(1); // hips

	pAnim->Init(0);
}

