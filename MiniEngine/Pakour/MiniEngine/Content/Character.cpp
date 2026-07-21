#include "pch.h"
#include "Character.h"

#include "Core/Math.h"
#include "Core/Log.h"
#include "Physics/CollsionLayer.h"
#include "Platform/Input.h"
#include "Manager/AssetManager.h"
#include "Manager/PathManager.h"

#include "Scene/Scene.h"
#include "Scene/CameraComponent.h"
#include "Scene/RigidBodyComponent.h"
#include "Scene/SkeletalMeshComponent.h"
#include "Scene/CharacterControllerComponent.h"
#include "Scene/PerceptionComponent.h"
#include "Animation/Animator.h"
#include "Animation/BlendClip.h"
#include "Animation/ActionClip.h"
#include "Animation/AnimNotify.h"

#include "Content/ContentConfig.h"

#include "Content/JumpTiming.h"
#include "Content/EnableCollisionObstacle.h"
#include "Content/TransitionState.h"
#include "Content/CorrectRootMotion.h"

#include "Content/CharacterStateMachine.h"
#include "Content/CharacterState/LandingState.h"
#include "Content/CharacterState/InAirState.h"
#include "Content/CharacterState/HangingState.h"

using namespace Content::Config;

Character::Character()
{
}
Character::~Character()
{
}

void Character::Construct()
{
	std::shared_ptr<SceneComponent> pRoot = AddComponent<SceneComponent>();
	pRoot->localTransform.position = Vector3(0.0f, 1.0f, 0.0f);
	pRoot->localTransform.rotation = Quaternion::CreateFromYawPitchRoll(ToRadians(180.0f), 0.0f, 0.0f);

	m_skinMeshComp = AddComponent<SkeletalMeshComponent>();
	PathManager* pathMgr = PathManager::GetInstance();

	std::wstring miniPath = pathMgr->ResolveAssetPath(L"Character.mini");
	std::shared_ptr<SkinnedMesh> skinnedMesh = AssetManager::GetInstance()->LoadSkinnedMesh(miniPath);

	std::shared_ptr<SkeletalMeshComponent> skinComp = GetSkin().lock();
	skinComp->SetMesh(skinnedMesh);
	skinComp->AttachTo(GetRoot());

	InitAnimation(skinComp);

	{
		// 캐릭터 카메라 설정
		std::shared_ptr<SceneComponent> pCamHolder = AddComponent<SceneComponent>();
		pCamHolder->AttachTo(GetRoot());
		pCamHolder->localTransform.position = Vector3(0.0f, 1.5f, 0.0f);

		std::shared_ptr<CameraComponent> pCamComp = AddComponent<CameraComponent>();
		pCamComp->RegisterMainCamera();

		pCamComp->AttachTo(pCamHolder);
		pCamComp->localTransform.position = Vector3(0.0f, 0.0f, -5.0f);
		pCamComp->localTransform.rotation = Quaternion::CreateFromYawPitchRoll(0.0f, 0.0f, ToRadians(180.0f));

		m_cameraHolder = pCamHolder;
	}
	{
		// 캐릭터 컨트롤러 설정
		std::shared_ptr<CharacterControllerComponent> pCharCont = AddComponent<CharacterControllerComponent>();
		Physics::CapsuleControllerDesc desc;
		desc.radius = m_capsuleRadius;
		desc.height = m_capsuleHeight;
		desc.stepOffset = 0.2f;
		desc.contactOffset = 0.05f;
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
				std::make_shared<HangingState>()
			});
		m_charFSM = pCharFSM;
	}

	PostConstruct();
}

void Character::PostConstruct()
{
	// 컴포넌트들 초기화 이후 호출
	// 컴포넌트 간 연결 사항 정리
	std::shared_ptr<Animator> pAnim = GetAnim().lock();
	pAnim->SetOverrideTrackStartEvent([this]() { GetController().lock()->SetCheckFalling(false); });
	pAnim->SetOverrideTrackEndEvent([this]() { GetController().lock()->SetCheckFalling(true); });
}

void Character::InitAnimation(std::shared_ptr<SkeletalMeshComponent>& _skinComp)
{
	// 애니메이션 설정
	std::shared_ptr<Animator> pAnim = _skinComp->GetAnim().lock();
	std::shared_ptr<MiniEngine::SkinnedMesh> skinnedMesh = _skinComp->GetMesh().lock();
	
	pAnim->ReserveBaseLocomotion(static_cast<uint8_t>(EState::END));

	// TODO : 데이터화
	// 로코모션 구현 (순서 유의 - EState 값 순서대로 할당하고 찾을 것)
	{
		// Landing
		std::shared_ptr<BlendClip> pBlend = std::make_shared<BlendClip>(9);

		// 모션 입력 
		pBlend->AddAnimClip({ 0, 0 }, skinnedMesh->GetClipPtr(1));	// idle
		pBlend->AddAnimClip({ 0, 1 }, skinnedMesh->GetClipPtr(5));		// run Fwd
		pBlend->AddAnimClip({ 0.5, 1 }, skinnedMesh->GetClipPtr(5));	// run Fwd
		pBlend->AddAnimClip({ -0.5, 1 }, skinnedMesh->GetClipPtr(5));	// run Fwd
		pBlend->AddAnimClip({ 0, -1 }, skinnedMesh->GetClipPtr(8));		// run Bwd
		pBlend->AddAnimClip({ 0.5, -1 }, skinnedMesh->GetClipPtr(8));	// run Bwd
		pBlend->AddAnimClip({ -0.5, -1 }, skinnedMesh->GetClipPtr(8));	// run Bwd
		pBlend->AddAnimClip({ -1, 0 }, skinnedMesh->GetClipPtr(6));	// left strafe
		pBlend->AddAnimClip({ 1, 0 }, skinnedMesh->GetClipPtr(7));	// right strafe
		
		pAnim->AddBaseLocomotion(pBlend);
	}
	{
		// InAir
		std::shared_ptr<BlendClip> pBlend = std::make_shared<BlendClip>(1);
		// 모션 입력 
		pBlend->AddAnimClip({ 0, 0 }, skinnedMesh->GetClipPtr(10));	// Falling Idle
		pAnim->AddBaseLocomotion(pBlend);
	}
	{
		// Hanging
		std::shared_ptr<BlendClip> pBlend = std::make_shared<BlendClip>(1);
		// 모션 입력 
		// 블렌드 모션이 생각보다 별로라 루트모션으로 대체
		pBlend->AddAnimClip({ 0, 0 }, skinnedMesh->GetClipPtr(18)); // Hanging Idle
		pAnim->AddBaseLocomotion(pBlend);
	}

	// ActionClip 구성
	{
		// 테스트 Jump
		std::shared_ptr<ActionClip> pActionClip = std::make_shared<ActionClip>();
		pActionClip->AddClip(skinnedMesh->GetClipPtr(9));
		pActionClip->SetApplyRootBone(false); // jump는 루트모션 적용하지 않음
		
		std::shared_ptr<JumpTiming> pJumpNotify = std::make_shared<JumpTiming>();
		pJumpNotify->SetTime(0.5f);
		pActionClip->AddNotify(pJumpNotify);

		m_mapActions[(uint8_t)Content::Config::ETagAct::Jump] = pActionClip;
	}
	{
		// Jump Valut
		// Vault Low, Mid
		std::shared_ptr<ActionClip> pActionClip = std::make_shared<ActionClip>();
		pActionClip->AddClip(skinnedMesh->GetClipPtr(12));

		std::shared_ptr<EnableCollisionObstacle> pIgnoreObstacle = std::make_shared<EnableCollisionObstacle>();
		pIgnoreObstacle->SetTime(0.15f);
		pIgnoreObstacle->SetEnable(false);
		pActionClip->AddNotify(pIgnoreObstacle);

		std::shared_ptr<EnableCollisionObstacle> pCollideObstacle = std::make_shared<EnableCollisionObstacle>();
		pCollideObstacle->SetTime(0.7f);
		pCollideObstacle->SetEnable(true);
		pActionClip->AddNotify(pCollideObstacle);

		std::shared_ptr<CorrectRootMotion> pCorrectRM = std::make_shared<CorrectRootMotion>();
		pCorrectRM->SetTime(0.0f, 0.4f);
		pCorrectRM->SetProperDistance(1.0f); // 적정거리 1.5 ~ 1.0
		pCorrectRM->SetLerpWeight(0.7f);
		pCorrectRM->SetDeltaIntensity(2.0f);
		pActionClip->AddNotify(pCorrectRM);
		
		m_mapActions[(uint8_t)ETagAct::VaultLow] = pActionClip;
		m_mapActions[(uint8_t)ETagAct::VaultMid] = pActionClip;
	}
	{
		// Vault High
		std::shared_ptr<ActionClip> pActionClip = std::make_shared<ActionClip>();
		pActionClip->AddClip(skinnedMesh->GetClipPtr(13));

		std::shared_ptr<EnableCollisionObstacle> pIgnoreObstacle = std::make_shared<EnableCollisionObstacle>();
		pIgnoreObstacle->SetTime(0.15f);
		pIgnoreObstacle->SetEnable(false);
		pActionClip->AddNotify(pIgnoreObstacle);

		std::shared_ptr<EnableCollisionObstacle> pCollideObstacle = std::make_shared<EnableCollisionObstacle>();
		pCollideObstacle->SetTime(1.0f);
		pCollideObstacle->SetEnable(true);
		pActionClip->AddNotify(pCollideObstacle);

		std::shared_ptr<CorrectRootMotion> pCorrectRM = std::make_shared<CorrectRootMotion>();
		pCorrectRM->SetTime(0.0f, 0.5f);
		pCorrectRM->SetProperDistance(1.0f); // 적정거리 1.5 ~ 1.0
		pCorrectRM->SetLerpWeight(0.7f);
		pActionClip->AddNotify(pCorrectRM);

		m_mapActions[(uint8_t)ETagAct::VaultHigh] = pActionClip;
	}
	{
		// Mantle_A_Jump_L_On
		// Mantle Low, Mid
		std::shared_ptr<ActionClip> pActionClip = std::make_shared<ActionClip>();
		pActionClip->AddClip(skinnedMesh->GetClipPtr(14));

		std::shared_ptr<EnableCollisionObstacle> pIgnoreObstacle = std::make_shared<EnableCollisionObstacle>();
		pIgnoreObstacle->SetTime(0.1f);
		pIgnoreObstacle->SetEnable(false);
		pActionClip->AddNotify(pIgnoreObstacle);

		std::shared_ptr<EnableCollisionObstacle> pCollideObstacle = std::make_shared<EnableCollisionObstacle>();
		pCollideObstacle->SetTime(0.5f);
		pCollideObstacle->SetEnable(true);
		pActionClip->AddNotify(pCollideObstacle);

		std::shared_ptr<CorrectRootMotion> pCorrectRM = std::make_shared<CorrectRootMotion>();
		pCorrectRM->SetCorrectAxis(CorrectRootMotion::ECorrectAxis::YZ);
		pCorrectRM->SetTime(0.2f, 0.6f);
		pCorrectRM->SetProperDistance(1.0f);
		pCorrectRM->SetLerpWeight(0.5f);
		pActionClip->AddNotify(pCorrectRM);
		
		m_mapActions[(uint8_t)ETagAct::MantleLow] = pActionClip;
		m_mapActions[(uint8_t)ETagAct::MantleMid] = pActionClip; 
	}
	{
		// Mantle_A_Wall_Monkey_On
		// Mantle High
		std::shared_ptr<ActionClip> pActionClip = std::make_shared<ActionClip>();
		pActionClip->AddClip(skinnedMesh->GetClipPtr(15));

		std::shared_ptr<EnableCollisionObstacle> pIgnoreObstacle = std::make_shared<EnableCollisionObstacle>();
		pIgnoreObstacle->SetTime(0.00f);
		pIgnoreObstacle->SetEnable(false);
		pActionClip->AddNotify(pIgnoreObstacle);

		std::shared_ptr<EnableCollisionObstacle> pCollideObstacle = std::make_shared<EnableCollisionObstacle>();
		pCollideObstacle->SetTime(0.9f);
		pCollideObstacle->SetEnable(true);
		pActionClip->AddNotify(pCollideObstacle);

		std::shared_ptr<CorrectRootMotion> pCorrectRM = std::make_shared<CorrectRootMotion>();
		pCorrectRM->SetCorrectAxis(CorrectRootMotion::ECorrectAxis::YZ);
		pCorrectRM->SetTime(0.0f, 0.6f);
		pCorrectRM->SetLerpWeight(0.5f);
		pCorrectRM->SetProperDistance(0.5f);
		pActionClip->AddNotify(pCorrectRM);

		m_mapActions[(uint8_t)ETagAct::MantleHigh] = pActionClip;
	}
	{
		// Falling To Landing
		// FallingToLand
		std::shared_ptr<ActionClip> pActionClip = std::make_shared<ActionClip>();
		pActionClip->AddClip(skinnedMesh->GetClipPtr(11));
		pActionClip->SetApplyRootBone(false);
		m_mapActions[(uint8_t)ETagAct::FallingToLand] = pActionClip;
	}
	{
		// Idle To Braced Hang
		// IdleToHang
		std::shared_ptr<ActionClip> pActionClip = std::make_shared<ActionClip>();
		pActionClip->AddClip(skinnedMesh->GetClipPtr(16)); // 벽 매달리기 (시작)

		std::shared_ptr<TransitionState> pSetHanging = std::make_shared<TransitionState>();
		pSetHanging->SetTime(0.1f);
		pSetHanging->SetState((uint8_t)EState::Hanging);
		pActionClip->AddNotify(pSetHanging);

		std::shared_ptr<CorrectRootMotion> pCorrectRM = std::make_shared<CorrectRootMotion>();
		pCorrectRM->SetTime(0.0f, 1.0f);
		pCorrectRM->SetProperDistance(0.05f);
		pCorrectRM->SetLerpWeight(0.95f);
		pActionClip->AddNotify(pCorrectRM);

		m_mapActions[(uint8_t)ETagAct::Wall_IdleToHang] = pActionClip;
	}
	{
		// Braced Hang Drop To Idle
		std::shared_ptr<ActionClip> pActionClip = std::make_shared<ActionClip>();
		pActionClip->AddClip(skinnedMesh->GetClipPtr(17)); // 벽에서 내려옴 (종료)
		pActionClip->SetApplyRootBone(false);

		std::shared_ptr<EnableCollisionObstacle> pIgnoreObstacle = std::make_shared<EnableCollisionObstacle>();
		pIgnoreObstacle->SetTime(0.1f);
		pIgnoreObstacle->SetEnable(false);
		pActionClip->AddNotify(pIgnoreObstacle);

		std::shared_ptr<EnableCollisionObstacle> pCollideObstacle = std::make_shared<EnableCollisionObstacle>();
		pCollideObstacle->SetTime(0.5f);
		pCollideObstacle->SetEnable(true);
		pActionClip->AddNotify(pCollideObstacle);

		std::shared_ptr<TransitionState> pSetLanding = std::make_shared<TransitionState>();
		pSetLanding->SetTime(0.5f);
		pSetLanding->SetState((uint8_t)EState::Landing);
		pActionClip->AddNotify(pSetLanding);

		m_mapActions[(uint8_t)ETagAct::Wall_HangToIdle] = pActionClip;
	}
	{
		// A_Ledge_ClimbUp_Monkey_Mantle
		std::shared_ptr<ActionClip> pActionClip = std::make_shared<ActionClip>();
		pActionClip->AddClip(skinnedMesh->GetClipPtr(28)); // 벽에서 올라감

		std::shared_ptr<TransitionState> pSetLanding = std::make_shared<TransitionState>();
		pSetLanding->SetTime(0.9f);
		pSetLanding->SetState((uint8_t)EState::Landing);
		pActionClip->AddNotify(pSetLanding);

		std::shared_ptr<EnableCollisionObstacle> pIgnoreObstacle = std::make_shared<EnableCollisionObstacle>();
		pIgnoreObstacle->SetTime(0.05f);
		pIgnoreObstacle->SetEnable(false);
		pActionClip->AddNotify(pIgnoreObstacle);

		std::shared_ptr<EnableCollisionObstacle> pCollideObstacle = std::make_shared<EnableCollisionObstacle>();
		pCollideObstacle->SetTime(0.7f);
		pCollideObstacle->SetEnable(true);
		pActionClip->AddNotify(pCollideObstacle);

		std::shared_ptr<CorrectRootMotion> pCorrectRM = std::make_shared<CorrectRootMotion>();
		pCorrectRM->SetCorrectAxis(CorrectRootMotion::ECorrectAxis::YZ);
		pCorrectRM->SetTime(0.0f, 0.5f);
		pCorrectRM->SetLerpWeight(0.5f);
		pCorrectRM->SetProperDistance(0.85f);
		pActionClip->AddNotify(pCorrectRM);

		m_mapActions[(uint8_t)ETagAct::Wall_HangToMantle] = pActionClip;
	}
	{
		// Wall_Hanging 중 이동 모션
		std::shared_ptr<ActionClip> pHangMoveUp = std::make_shared<ActionClip>();
		pHangMoveUp->AddClip(skinnedMesh->GetClipPtr(19));
		m_mapActions[(uint8_t)ETagAct::Wall_HangingMoveUp] = pHangMoveUp;

		std::shared_ptr<ActionClip> pHangMoveDown = std::make_shared<ActionClip>();
		pHangMoveDown->AddClip(skinnedMesh->GetClipPtr(20));
		m_mapActions[(uint8_t)ETagAct::Wall_HangingMoveDown] = pHangMoveDown;

		std::shared_ptr<ActionClip> pHangMoveLeft = std::make_shared<ActionClip>();
		pHangMoveLeft->AddClip(skinnedMesh->GetClipPtr(21));
		m_mapActions[(uint8_t)ETagAct::Wall_HangingMoveLeft] = pHangMoveLeft;

		std::shared_ptr<ActionClip> pHangMoveRight = std::make_shared<ActionClip>();
		pHangMoveRight->AddClip(skinnedMesh->GetClipPtr(22));
		m_mapActions[(uint8_t)ETagAct::Wall_HangingMoveRight] = pHangMoveRight;
	}
	{
		// A_Ledge_Jump180_L
		// 벽에서 매달린 상태에서 점프
		std::shared_ptr<ActionClip> pActionClip = std::make_shared<ActionClip>();
		pActionClip->AddClip(skinnedMesh->GetClipPtr(23));

		// 벽에서 매달렸을 것. 상태 전환 -> Jump로 전환이므로 InAir
		std::shared_ptr<TransitionState> pSetInAir = std::make_shared<TransitionState>();
		pSetInAir->SetTime(0.3f);
		pSetInAir->SetState((uint8_t)EState::InAir);
		pActionClip->AddNotify(pSetInAir);

		m_mapActions[(uint8_t)ETagAct::JumpFromWall] = pActionClip;
	}
	{
		// 벽에서 코너 돌기 Inner Left
		std::shared_ptr<ActionClip> pActionClip = std::make_shared<ActionClip>();
		pActionClip->AddClip(skinnedMesh->GetClipPtr(24));

		m_mapActions[(uint8_t)ETagAct::Wall_InnerRotateLeft] = pActionClip;
	}
	{
		// 벽에서 코너 돌기 Inner Right
		std::shared_ptr<ActionClip> pActionClip = std::make_shared<ActionClip>();
		pActionClip->AddClip(skinnedMesh->GetClipPtr(25));

		m_mapActions[(uint8_t)ETagAct::Wall_InnerRotateRight] = pActionClip;
	}
	{
		// 벽에서 코너 돌기 Outer Left
		std::shared_ptr<ActionClip> pActionClip = std::make_shared<ActionClip>();
		pActionClip->AddClip(skinnedMesh->GetClipPtr(26));

		m_mapActions[(uint8_t)ETagAct::Wall_OuterRotateLeft] = pActionClip;
	}
	{
		// 벽에서 코너 돌기 Outer Right
		std::shared_ptr<ActionClip> pActionClip = std::make_shared<ActionClip>();
		pActionClip->AddClip(skinnedMesh->GetClipPtr(27));

		m_mapActions[(uint8_t)ETagAct::Wall_OuterRotateRight] = pActionClip;
	}
	{
		// 애니메이션 테스트
		std::shared_ptr<ActionClip> pActionClip = std::make_shared<ActionClip>();
		pActionClip->AddClip(skinnedMesh->GetClipPtr(28));

		m_mapActions[(uint8_t)Content::Config::ETagAct::Test] = pActionClip;
	}

	pAnim->SetEnableRootMotion(true);

	RootMotionConfig rmCfg;
	rmCfg.extractY = true;
	rmCfg.extractYaw = false;
	rmCfg.applyY = true;
	rmCfg.applyYaw = false;

	pAnim->SetRootMotionConfig(rmCfg);
	pAnim->SetRootBoneIdx(1); // hips

	pAnim->Init(0);
}

void Character::BeginPlay()
{
	Actor::BeginPlay();

	InitCollisionLayer();
	InitInput();
}

void Character::Tick(float _dt)
{
	Actor::Tick(_dt);
}

void Character::ProcessPerceptionResult()
{
	if (GetAnim().lock()->IsActionClipPlaying((uint8_t)EActionPriority::Override))
		return; // 이미 행동 중이라면 탐색하지 않도록

	std::shared_ptr<PerceptionComponent> pPercept = m_perception.lock();
	pPercept->Travel(); // 탐색 개시

	const TravelResult& result = pPercept->GetLastestTravelResult(); // 탐색 결과 확인
	if (result.m_bIsEmpty) // 빈 결과는 리턴
		return;

	std::shared_ptr<ActionClip> pAction = GetActions(result.m_actTag);
	
	if (result.m_pFirstObstacle)
	{
		m_pCurObstacle = reinterpret_cast<Actor*>(result.m_pFirstObstacle);
		m_curObstacleHitPos = result.m_firstObstacleHitPos;
		m_curObstacleDistance = result.m_distanceObstacle;
		m_curObstacleLedge = result.m_obstacleLedge;

		// MG_LOG_INFO("[Character] Check Ledge : {}", m_curObstacleLedge);
	}
	else
	{
		m_pCurObstacle = nullptr;
		if (result.m_bIsEmpty == false)
			MG_LOG_WARN("[Character] Travel Result returned but CurObstacle is null");
	}

	if (pAction)
		GetAnim().lock()->PlayActionClip(pAction, 0.2f, (uint8_t)EActionPriority::Override);
}


std::weak_ptr<Animator> Character::GetAnim() const
{
	return m_skinMeshComp.lock()->GetAnim();
}

void Character::SetEnableCollisionObstacle(bool _bEnable)
{
	std::shared_ptr<CharacterControllerComponent> pCharCont = GetController().lock();
	pCharCont->SetLayerCollisionEnabled(MiniEngine::Physics::Layer::Obstacle, _bEnable);
}

void Character::AddMovementInput(const Vector3& _moveDelta)
{
	m_charCont.lock()->AddMovementInput(_moveDelta);
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

void Character::PlayActionClip(std::shared_ptr<ActionClip> _clip, float _transitionTime)
{
	if (_clip == nullptr)
		return;

	GetAnim().lock()->PlayActionClip(_clip, _transitionTime);
}

void Character::TransitionStateMachine(uint8_t _state)
{
	SetState(static_cast<Character::EState>(_state));
	m_charFSM.lock()->Transition(_state);
}

Vector3 Character::GetCurObstacleHitPos() const
{
	Vector3 hitPos = m_curObstacleHitPos;
	hitPos.y = GetCurObstacleLedge();
	return hitPos;
}

void Character::InitCollisionLayer()
{
	m_charCont.lock()->SetLayerCollisionEnabled(MiniEngine::Physics::Layer::ObstacleLedge, false);
}

void Character::InitInput()
{
	ResetCamRot();

	// 바인딩
	Input& input = InputManager::GetInstance()->GetInput();

	input.GetKeyBind(DirectX::Keyboard::Keys::Escape).OnPressed = std::bind([this]() { PostQuitMessage(0); });
	input.GetKeyBind(DirectX::Keyboard::Keys::W).OnPressed = std::bind(
		[this]()
		{
			Vector2 inputDir = GetInputDir();
			inputDir.y = 1.0f;
			SetInputDir(inputDir);
		});
	input.GetKeyBind(DirectX::Keyboard::Keys::W).OnReleased = std::bind(
		[this]()
		{
			Vector2 inputDir = GetInputDir();
			inputDir.y = 0.0f;
			SetInputDir(inputDir);
		});

	input.GetKeyBind(DirectX::Keyboard::Keys::S).OnPressed = std::bind(
		[this]()
		{
			Vector2 inputDir = GetInputDir();
			inputDir.y = -1.0f;
			SetInputDir(inputDir);
		});
	input.GetKeyBind(DirectX::Keyboard::Keys::S).OnReleased = std::bind(
		[this]()
		{
			Vector2 inputDir = GetInputDir();
			inputDir.y = 0.0f;
			SetInputDir(inputDir);
		});

	input.GetKeyBind(DirectX::Keyboard::Keys::D).OnPressed = std::bind(
		[this]()
		{
			Vector2 inputDir = GetInputDir();
			inputDir.x = 1.0f;
			SetInputDir(inputDir);
		});
	input.GetKeyBind(DirectX::Keyboard::Keys::D).OnReleased = std::bind(
		[this]()
		{
			Vector2 inputDir = GetInputDir();
			inputDir.x = 0.0f;
			SetInputDir(inputDir);
		});

	input.GetKeyBind(DirectX::Keyboard::Keys::A).OnPressed = std::bind(
		[this]()
		{
			Vector2 inputDir = GetInputDir();
			inputDir.x = -1.0f;
			SetInputDir(inputDir);
		});
	input.GetKeyBind(DirectX::Keyboard::Keys::A).OnReleased = std::bind(
		[this]()
		{
			Vector2 inputDir = GetInputDir();
			inputDir.x = 0.0f;
			SetInputDir(inputDir);
		});

	// 테스트용 점프
	input.GetKeyBind(DirectX::Keyboard::Keys::Space).OnReleased = std::bind(
		[this]() { InputJump(); }
	);
	input.GetKeyBind(DirectX::Keyboard::Keys::LeftShift).OnPressed = std::bind(
		[this]() { ProcessPerceptionResult();  }
	);
	input.GetKeyBind(DirectX::Keyboard::Keys::F3).OnPressed = std::bind(
		[this]() { ResetCamRot(); }
	);
	input.GetKeyBind(DirectX::Keyboard::Keys::Q).OnPressed = std::bind(
		[this]() 
		{
			std::shared_ptr<ActionClip> pTest = GetActions((uint8_t)Content::Config::ETagAct::Test);
			GetAnim().lock()->PlayActionClip(pTest, 0.2f);
		}
	);
}