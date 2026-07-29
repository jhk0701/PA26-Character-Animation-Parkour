#include "pch.h"
#include "Character.h"

#include "Core/Math.h"
#include "Core/Log.h"
#include "Core/DebugMarkers.h"

#include "Physics/CollsionLayer.h"
#include "Platform/Input.h"
#include "Manager/AssetManager.h"
#include "Manager/PathManager.h"

#include "Scene/Scene.h"
#include "Scene/RigidBodyComponent.h"
#include "Scene/SkeletalMeshComponent.h"
#include "Scene/CharacterControllerComponent.h"
#include "Scene/PerceptionComponent.h"
#include "Animation/Animator.h"

#include "Content/ContentConfig.h"
#include "Content/ActionClipContainer.h"

#include "Content/CharacterStateMachine.h"
#include "Content/CharacterState/LandingState.h"
#include "Content/CharacterState/InAirState.h"
#include "Content/CharacterState/HangingState.h"
#include "Content/CharacterState/BeamState.h"

using namespace Content::Config;

Character::Character()
{
}
Character::~Character()
{
}

void Character::Construct(const Vector3& _initPosition)
{
	std::shared_ptr<SceneComponent> pRoot = AddComponent<SceneComponent>();
	m_skinMeshComp = AddComponent<SkeletalMeshComponent>();
	PathManager* pathMgr = PathManager::GetInstance();

	std::wstring miniPath = pathMgr->ResolveAssetPath(L"Character.mini");
	std::shared_ptr<SkinnedMesh> skinnedMesh = AssetManager::GetInstance()->LoadSkinnedMesh(miniPath);

	std::shared_ptr<SkeletalMeshComponent> skinComp = GetSkin().lock();
	skinComp->SetMesh(skinnedMesh);
	skinComp->AttachTo(GetRoot());

	InitAnimation(skinComp);

	{
		std::shared_ptr<LimbIKComponent> pLimbIK = AddComponent<LimbIKComponent>();
		LimbIKDesc desc;
		desc.footHeight = 0.0f;
		desc.maxFootDrop = 0.4f;
		desc.maxFootRaise = 0.4f;
		desc.maxPelvisDrop = 0.45f;

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
		pCharCont->SetFallingSecondThreshold(0.3f); // 낙하 인정 시간 설정

		m_charCont = pCharCont;
	}
	{
		std::shared_ptr<PerceptionComponent> pPerceptComp = AddComponent<PerceptionComponent>();
		pPerceptComp->SetQueryTree(m_perceptQueryTree.ConstructTree());
		m_perception = pPerceptComp;
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
				std::make_shared<BeamHangingState>()
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

	InitCollisionLayer();
	m_charFSM.lock()->Start();
	m_charCont.lock()->SetCheckFalling(true);
}

void Character::TryPerception()
{
	if (GetAnim().lock()->IsActionClipPlaying((uint8_t)EActionPriority::Override))
		return; // 이미 행동 중이라면 탐색하지 않도록

	std::shared_ptr<PerceptionComponent> pPercept = m_perception.lock();
	pPercept->Travel(); // 탐색 개시

	const TravelResult& result = pPercept->GetLastestTravelResult(); // 탐색 결과 확인
	if (result.m_bIsEmpty) // 빈 결과는 리턴
		return;

	ProcessPerceptionResult(result);
}
void Character::ProcessPerceptionResult(const TravelResult& _result)
{
	if (_result.m_pFirstObstacle)
	{
		m_curObstacleInfo.m_actTag = _result.m_actTag;
		m_curObstacleInfo.m_pObstacle = _result.m_pFirstObstacle;
		m_curObstacleInfo.m_obstacleHitPos = _result.m_firstObstacleHitPos;
		m_curObstacleInfo.m_obstacleHitNrm = _result.m_firstObstacleHitNrm;
		m_curObstacleInfo.m_obstacleDistance = _result.m_distanceObstacle;
		m_curObstacleInfo.m_obstacleLedge = _result.m_obstacleLedge;
		// MG_LOG_INFO("[Character] Check Ledge : {}", m_curObstacleLedge);
	}
	else
	{
		m_curObstacleInfo.m_pObstacle = nullptr;
		MG_LOG_WARN("[Character] Travel Result returned but Cur Obstacle is null");
	}

	if (m_charFSM.expired())
		return;

	// 세부 처리는 각 상태일때 달리 처리
	m_charFSM.lock()->ProcessPerceptionResult(m_curObstacleInfo);
}

void Character::InitCollisionLayer()
{
	m_charCont.lock()->SetLayerCollisionEnabled(MiniEngine::Physics::Layer::ObstacleLedge, false);
}
void Character::SetEnableCollisionObstacle(bool _bEnable)
{
	m_charCont.lock()->SetLayerCollisionEnabled(MiniEngine::Physics::Layer::Obstacle, _bEnable);
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
	
	uint8_t tag = m_state == EState::Landing ? 
		(uint8_t)Content::Config::ETagAct::Jump :
		(uint8_t)Content::Config::ETagAct::JumpFromWall;

	if (std::shared_ptr<ActionClip> pJump = GetActions(tag))
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

void Character::ReserveIKDetectGround()
{
	if (m_limbIKComp.expired())
		return;

	std::shared_ptr<LimbIKComponent> pLimbIK = m_limbIKComp.lock();
	pLimbIK->SetEnableIK(ELimbType::LeftLeg, true);
	pLimbIK->SetEnableIK(ELimbType::RightLeg, true);
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
	param.m_dir = Vector3(0.0f, - 1.0f, 0.0f);
	param.m_maxDistance = m_ikRayDistance * 2.0f;
	param.m_origin = result.position;
	param.m_origin.y += m_ikRayDistance;

	Physics::RaycastResult hitResult;

	std::shared_ptr<Physics::PhysicsWorld> pPhysics = GetScene()->GetPhysics().lock();
	if (!pPhysics->Raycast(param, hitResult, Physics::Layer::Ground | Physics::Layer::Obstacle))
		return result;

	MiniEngine::Debug::DrawPoint(hitResult.m_pos, MiniEngine::DebugColor::YELLOW, 0.05f, MiniEngine::Debug::EMarkerShape::Sphere, 0.01f);

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
	// result.rotAlpha = 1.0f;
	return result;
}

void Character::IKDetectObstacle(uint8_t _ik)
{
	// 노티파이를 통해서 호출될 것
	// 파쿠르 중 장애물에 손, 발을 가져다 대는 용도
	// 이미 장애물의 위치 등을 알고 있기 때문에 레이캐스트를 쏘진 않을 것
	if (!m_curObstacleInfo.IsValid())
		return;

	Vector3 targetPos = m_curObstacleInfo.m_obstacleHitPos;
	targetPos.y = m_curObstacleInfo.m_obstacleLedge;

	MiniEngine::Debug::DrawPoint(targetPos, MiniEngine::DebugColor::YELLOW, 0.05f, MiniEngine::Debug::EMarkerShape::Sphere, 0.01f);
	// m_limbIKComp.lock()->SetTargetPosIK((ELimbType)_ik, targetPos);
}

void Character::ClearIKReserve()
{
	m_limbIKComp.lock()->ClearPendingTask();
	m_limbIKComp.lock()->SetEnableAllIK(false);
}

void Character::SetIKAlpha(uint8_t _ik, float _alpha)
{
	m_limbIKComp.lock()->SetAlphaIK((ELimbType)_ik, _alpha);
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

	ActionClipLoadParam param;
	param.pAnim = pAnim;
	param.pSources = _skinComp->GetMesh().lock();
	param.pMaps = &m_mapActions;
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