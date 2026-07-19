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
#include "Content/EnableHangingState.h"
#include "Content/CorrectRootMotion.h"

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

	std::wstring miniPath = pathMgr->ResolveAssetPath(L"Character_test.mini");
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
		pBlend->AddAnimClip({ 0, 1 }, skinnedMesh->GetClipPtr(6));	// Walking
		pBlend->AddAnimClip({ 0.5, 1 }, skinnedMesh->GetClipPtr(6));	// Walking
		pBlend->AddAnimClip({ -0.5, 1 }, skinnedMesh->GetClipPtr(6));	// Walking
		pBlend->AddAnimClip({ 0, -1 }, skinnedMesh->GetClipPtr(9));	// Walking Backword
		pBlend->AddAnimClip({ 0.5, -1 }, skinnedMesh->GetClipPtr(9));	// Walking Backword
		pBlend->AddAnimClip({ -0.5, -1 }, skinnedMesh->GetClipPtr(9));	// Walking Backword
		pBlend->AddAnimClip({ 1, 0 }, skinnedMesh->GetClipPtr(8));	// right strafe
		pBlend->AddAnimClip({ -1, 0 }, skinnedMesh->GetClipPtr(7));	// left strafe
		pAnim->AddBaseLocomotion(pBlend);
	}
	{
		// InAir
		std::shared_ptr<BlendClip> pBlend = std::make_shared<BlendClip>(1);
		// 모션 입력 
		pBlend->AddAnimClip({ 0, 0 }, skinnedMesh->GetClipPtr(13));	// Falling Idle
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
		pActionClip->AddClip(skinnedMesh->GetClipPtr(10));
		pActionClip->SetApplyRootBone(false); // jump는 루트모션 적용하지 않음
		
		std::shared_ptr<JumpTiming> pJumpNotify = std::make_shared<JumpTiming>();
		pJumpNotify->SetTime(0.5f);
		pActionClip->AddNotify(pJumpNotify);

		m_mapActions[(uint8_t)Content::Config::ETagAct::Jump] = pActionClip;
	}
	{
		// Jump Valut
		std::shared_ptr<ActionClip> pActionClip = std::make_shared<ActionClip>();
		pActionClip->AddClip(skinnedMesh->GetClipPtr(11)); // Jumping valut

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

		m_mapActions[(uint8_t)ETagAct::VaultMid] = pActionClip;
		m_mapActions[(uint8_t)ETagAct::VaultLow] = pActionClip; // Valut 낮은 모션 적용 
		m_mapActions[(uint8_t)ETagAct::MantleLow] = pActionClip; // TODO : 중간 지점 오르는 애니메이션 필요
	}
	{
		// Mantle Mid
		std::shared_ptr<ActionClip> pActionClip = std::make_shared<ActionClip>();
		pActionClip->AddClip(skinnedMesh->GetClipPtr(12)); // Sprint To Wall Climb

		std::shared_ptr<EnableCollisionObstacle> pIgnoreObstacle = std::make_shared<EnableCollisionObstacle>();
		pIgnoreObstacle->SetTime(0.4f);
		pIgnoreObstacle->SetEnable(false);
		pActionClip->AddNotify(pIgnoreObstacle);

		std::shared_ptr<EnableCollisionObstacle> pCollideObstacle = std::make_shared<EnableCollisionObstacle>();
		pCollideObstacle->SetTime(0.7f);
		pCollideObstacle->SetEnable(true);
		pActionClip->AddNotify(pCollideObstacle);


		m_mapActions[(uint8_t)ETagAct::MantleMid] = pActionClip; // Mantle_Mid 넘어 오르는 모션
	}
	{
		// FallingToLand
		std::shared_ptr<ActionClip> pActionClip = std::make_shared<ActionClip>();
		pActionClip->AddClip(skinnedMesh->GetClipPtr(14));
		pActionClip->SetApplyRootBone(false);
		m_mapActions[(uint8_t)ETagAct::FallingToLand] = pActionClip;
	}
	{
		// IdleToHang
		std::shared_ptr<ActionClip> pActionClip = std::make_shared<ActionClip>();
		pActionClip->AddClip(skinnedMesh->GetClipPtr(15)); // 벽 매달리기 (시작)

		std::shared_ptr<EnableHangingState> pSetHanging = std::make_shared<EnableHangingState>();
		pSetHanging->SetTime(0.2f);
		pSetHanging->SetEnable(true);
		pActionClip->AddNotify(pSetHanging);

		std::shared_ptr<CorrectRootMotion> pCorrectRM = std::make_shared<CorrectRootMotion>();
		pCorrectRM->SetTime(0.0f, 1.0f);
		pCorrectRM->SetProperDistance(0.05f);
		pCorrectRM->SetLerpWeight(0.95f);
		pActionClip->AddNotify(pCorrectRM);

		m_mapActions[(uint8_t)ETagAct::IdleToHang] = pActionClip;
	}
	{
		// HangToIdle
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

		std::shared_ptr<EnableHangingState> pSetIdle = std::make_shared<EnableHangingState>();
		pSetIdle->SetTime(0.5f);
		pSetIdle->SetEnable(false);
		pActionClip->AddNotify(pSetIdle);

		std::shared_ptr<CorrectRootMotion> pCorretRM = std::make_shared<CorrectRootMotion>();
		pCorretRM->SetTime(0.0f, 0.5f);
		pCorretRM->SetProperDistance(0.01f);
		pCorretRM->SetLerpWeight(0.8f);
		pCorretRM->SetDeltaIntensity(5.0f);
		pCorretRM->SetCorrectAxis(CorrectRootMotion::YZ);
		pActionClip->AddNotify(pCorretRM);

		m_mapActions[(uint8_t)ETagAct::HangToIdle] = pActionClip;
	}
	{
		// HangToIdle
		std::shared_ptr<ActionClip> pActionClip = std::make_shared<ActionClip>();
		pActionClip->AddClip(skinnedMesh->GetClipPtr(16)); // 벽에서 올라감

		std::shared_ptr<EnableHangingState> pSetIdle = std::make_shared<EnableHangingState>();
		pSetIdle->SetTime(0.5f);
		pSetIdle->SetEnable(false);
		pActionClip->AddNotify(pSetIdle);

		std::shared_ptr<EnableCollisionObstacle> pIgnoreObstacle = std::make_shared<EnableCollisionObstacle>();
		pIgnoreObstacle->SetTime(0.1f);
		pIgnoreObstacle->SetEnable(false);
		pActionClip->AddNotify(pIgnoreObstacle);

		std::shared_ptr<EnableCollisionObstacle> pCollideObstacle = std::make_shared<EnableCollisionObstacle>();
		pCollideObstacle->SetTime(0.5f);
		pCollideObstacle->SetEnable(true);
		pActionClip->AddNotify(pCollideObstacle);

		std::shared_ptr<CorrectRootMotion> pCorrectRM = std::make_shared<CorrectRootMotion>();
		pCorrectRM->SetTime(0.0f, 0.5f);
		pCorrectRM->SetLerpWeight(0.99f);
		pCorrectRM->SetDeltaIntensity(10.0f);
		pCorrectRM->SetProperDistance(0.05f);
		pActionClip->AddNotify(pCorrectRM);

		m_mapActions[(uint8_t)ETagAct::HangToMantle] = pActionClip;
	}
	{
		// Hanging 중 이동 모션
		std::shared_ptr<ActionClip> pHangMoveUp = std::make_shared<ActionClip>();
		pHangMoveUp->AddClip(skinnedMesh->GetClipPtr(20));
		m_mapActions[(uint8_t)ETagAct::HangingMoveUp] = pHangMoveUp;

		std::shared_ptr<ActionClip> pHangMoveDown = std::make_shared<ActionClip>();
		pHangMoveDown->AddClip(skinnedMesh->GetClipPtr(19));
		m_mapActions[(uint8_t)ETagAct::HangingMoveDown] = pHangMoveDown;

		std::shared_ptr<ActionClip> pHangMoveRight = std::make_shared<ActionClip>();
		pHangMoveRight->AddClip(skinnedMesh->GetClipPtr(21));
		m_mapActions[(uint8_t)ETagAct::HangingMoveRight] = pHangMoveRight;

		std::shared_ptr<ActionClip> pHangMoveLeft = std::make_shared<ActionClip>();
		pHangMoveLeft->AddClip(skinnedMesh->GetClipPtr(22));
		m_mapActions[(uint8_t)ETagAct::HangingMoveLeft] = pHangMoveLeft;
	}
	{
		// 벽에서 매달린 상태에서 점프
		std::shared_ptr<ActionClip> pActionClip = std::make_shared<ActionClip>();
		pActionClip->AddClip(skinnedMesh->GetClipPtr(23)); // Jump From Wall

		// 벽에서 매달렸을 것. 상태 전환
		std::shared_ptr<EnableHangingState> pSetIdle = std::make_shared<EnableHangingState>();
		pSetIdle->SetTime(0.5f);
		pSetIdle->SetEnable(false);
		pActionClip->AddNotify(pSetIdle);

		m_mapActions[(uint8_t)Content::Config::ETagAct::JumpFromWall] = pActionClip;
	}
	{
		// 벽에서 매달린 상태에서 점프
		std::shared_ptr<ActionClip> pActionClip = std::make_shared<ActionClip>();
		pActionClip->AddClip(skinnedMesh->GetClipPtr(24)); // Jump From Wall

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
	// TODO : FSM으로 리팩터링
	InputCamRotate();
	InputMovement(_dt);
	
	Actor::Tick(_dt);

	// 판단 절차
	// 내용 적용은 다음 tick에서 반영
	CheckCharacterState();
}

void Character::InputMovement(float _dt)
{
	// 키보드 이동키
	if (GetAnim().lock()->IsActionClipPlaying())
		return;

	// TODO : 전체 구조에서 FSM 고려해볼 것
	switch (m_state)
	{
	case EState::Hanging: 
		{
			// 4 방향 중 하나만 골라야 함
			ETagAct eAct = ETagAct::End;
			if (m_inputDir.y > 0)
				eAct = ETagAct::HangingMoveUp;
			else if (m_inputDir.y < 0)
				eAct = ETagAct::HangingMoveDown;
			else if (m_inputDir.x < 0)
				eAct = ETagAct::HangingMoveRight;
			else if (m_inputDir.x > 0)
				eAct = ETagAct::HangingMoveLeft;

			if (std::shared_ptr<ActionClip> pAct = m_mapActions[(uint8_t)eAct])
				GetAnim().lock()->PlayActionClip(pAct, 0.1f);

			break;
		}
	case EState::InAir: 
		{ break; }
	case EState::Landing: __fallthrough;
	default:
		{
			Vector2 inputDir = m_inputDir;
			inputDir.Normalize();
			m_lerpInputDir = Vector2::Lerp(m_lerpInputDir, inputDir, m_lerpWeight * _dt);

			const float deltaSpeed = _dt * m_moveSpeed;
			std::shared_ptr<SceneComponent> pRoot = GetRoot();

			// 캐릭터 정면 기준 이동
			const Vector3& fwd = pRoot->localTransform.Forward();
			const Vector3& rht = pRoot->localTransform.Right();
			m_charCont.lock()->AddMovementInput(deltaSpeed * m_lerpInputDir.y * fwd + deltaSpeed * -m_lerpInputDir.x * rht);

			GetAnim().lock()->SetBaseTrackInputAxis(m_lerpInputDir);
			break;
		}
	}
}

void Character::InputCamRotate()
{
	Input& input = InputManager::GetInstance()->GetInput();

	// 마우스 델타에 이미 델타타임이 곱해져 있음
	const Vector2 camRotSpeed = m_camRotateSpeed * input.GetMouseDelta();
	m_camRotate.x += camRotSpeed.x;
	m_camRotate.y += camRotSpeed.y;
	m_camRotate.y = std::clamp(m_camRotate.y, 180.0f - m_camPitchMaxDeg, 180.0f + m_camPitchMaxDeg);

	Quaternion qYaw = Quaternion::CreateFromAxisAngle(Vector3::Transform(Vector3(.0f, 1.0f, .0f), Quaternion(0.0f, 0.0f, 0.0f, 1.0f)), ToRadians(m_camRotate.x));
	Quaternion qPitch = Quaternion::CreateFromAxisAngle(Vector3(1.0f, 0.0f, 0.0f), ToRadians(m_camRotate.y));
	qYaw.Normalize();
	qPitch.Normalize();

	std::shared_ptr<SceneComponent> pCamHolderRoot = m_cameraHolder.lock();

	GetRoot()->localTransform.rotation = qYaw;
	pCamHolderRoot->localTransform.rotation = qPitch;
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

		if (result.m_actTag >= (uint8_t)Content::Config::ETagAct::Mantle)
		{
			MG_LOG_INFO("[Character] Find Ledge Value : {}", result.m_obstacleLedge);
		}
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

void Character::CheckCharacterState()
{
	if (m_state == EState::Hanging) // PerceptionComp에서 인식하고 반영해줄 것
		return; // 벽에 매달린 상황일 때

	std::shared_ptr<Animator> pAnim = GetAnim().lock();
	std::shared_ptr<CharacterControllerComponent> pCharCont = m_charCont.lock();

	if (!pAnim || !pCharCont)
		return;
	
	// 애니메이션 baseTrack의 상태만 전환하기 위한 용도
	// 공중인지 판단
	const bool bIsGrounded = pCharCont->IsGrounded();	// 땅에 닿았는지
	const bool bIsFalling = pCharCont->IsFalling();		// 실질적으로 떨어지고 있는지

	if (bIsFalling && 
		m_state != EState::InAir)
	{
		m_state = EState::InAir;
		pAnim->TranstionBaseTrack(static_cast<uint8_t>(m_state), 0.25f);
		return;
	}

	if (bIsGrounded && m_state != EState::Landing)
	{
		if (m_state == EState::InAir)
		{
			// 공중 -> 착지
			if (std::shared_ptr<ActionClip> pClip = m_mapActions[(uint8_t)Content::Config::ETagAct::FallingToLand])
			{
				pAnim->PlayActionClip(pClip, 0.2f);
			}
		}

		m_state = EState::Landing;
		pAnim->TranstionBaseTrack(static_cast<uint8_t>(m_state), 0.25f);

		return;
	}
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

void Character::Jump()
{
	m_charCont.lock()->Jump(m_jumpSpeed);
}

void Character::InputJump()
{
	// TODO : FSM 정리 대상
	if (m_state == EState::InAir)
		return;
	
	uint8_t tag = m_state == EState::Landing ? 
		(uint8_t)Content::Config::ETagAct::Jump :
		(uint8_t)Content::Config::ETagAct::JumpFromWall;

	std::shared_ptr<ActionClip> pJump = GetActions(tag);
	GetAnim().lock()->PlayActionClip(pJump, 0.2f);
}

void Character::SetHangingState(bool _bIsOn)
{
	SetState(_bIsOn ? EState::Hanging : EState::Landing);

	std::shared_ptr<CharacterControllerComponent> pCharCont = m_charCont.lock();
	pCharCont->SetUseGravity(_bIsOn ? false : true); // 매달린 중에는 중력 적용 해제
	pCharCont->SetForceFalling(false);

	GetAnim().lock()->TranstionBaseTrack(static_cast<uint8_t>(m_state), 0.25f);
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
		[this]()
		{
			InputJump();
		});
	input.GetKeyBind(DirectX::Keyboard::Keys::LeftShift).OnPressed = std::bind(
		[this]() { ProcessPerceptionResult();  }
	);
	input.GetKeyBind(DirectX::Keyboard::Keys::F3).OnPressed = std::bind(
		[this]() { ResetCamRot(); }
	);

	input.GetKeyBind(DirectX::Keyboard::Keys::Q).OnPressed = std::bind(
		[this]() 
		{ 
			const Transform& tf = GetRoot()->localTransform;
			MiniEngine::Physics::SpherecastParam spParam;
			spParam.m_dir = tf.Forward();
			spParam.m_maxDistance = 1.0f;
			spParam.m_startPos = tf.position + Vector3(0.0f, 1.0f, 0.0f) * GetCapsuleHalfHeight();
			spParam.m_radius = 0.5f;
			
			MiniEngine::Physics::RaycastResult spResult;
			bool isHit = GetScene()->GetPhysics().lock()->SphereCast(spParam, spResult, MiniEngine::Physics::ToMask(MiniEngine::Physics::Layer::ObstacleLedge));

			if (isHit)
				MG_LOG_INFO("[Test] : Get Ledge by sphere cast :: {}, {}, {}",
					spResult.m_pos.x, spResult.m_pos.y, spResult.m_pos.z);
		}
	);
}