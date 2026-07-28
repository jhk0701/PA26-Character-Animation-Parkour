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
#include "Animation/IK/FootIKComponent.h"

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
	{
		// 캐릭터 IK 설정
		{
			m_hLeftLeg.m_binding.upper = HumanoidBone::LeftUpperLeg;
			m_hLeftLeg.m_binding.lower = HumanoidBone::LeftLowerLeg;
			m_hLeftLeg.m_binding.end = HumanoidBone::LeftFoot;
			m_hLeftLeg.m_targetPos = GetRoot()->localTransform.position + Vector3(-0.3f, 0.0f, 0.0f);
		}
		
		// std::shared_ptr<FootIKComponent> pFootIK = AddComponent<FootIKComponent>();
		// m_footIK = pFootIK;
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

void Character::BeginPlay()
{
	Pawn::BeginPlay();

	m_charCont.lock()->SetCheckFalling(true);

	InitCollisionLayer();
}

void Character::OnBeforeSortComponent()
{
	Pawn::OnBeforeSortComponent();

	// 컴포넌트 우선순위 설정
	m_skinMeshComp.lock()->SetSortOrder(2); 
}

void Character::Tick(float _dt)
{
	Pawn::Tick(_dt);

	GetSkin().lock()->SetIKGoalWorld(m_hLeftLeg.m_binding, m_hLeftLeg.m_targetPos, m_hLeftLeg.m_alpha);
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
	m_charCont.lock()->SetPosition(_newPos + Vector3(0.0f, GetCapsuleHalfHeight(), 0.0f));
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