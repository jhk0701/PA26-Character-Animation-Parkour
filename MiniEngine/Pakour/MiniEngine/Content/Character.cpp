#include "pch.h"
#include "Character.h"

#include "Content/ContentConfig.h"
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


Character::Character()
{
}
Character::~Character()
{
}

void Character::Construct()
{
	std::shared_ptr<SceneComponent> pRoot = AddComponent<SceneComponent>();
	pRoot->localTransform.position = Vector3(0.0f, 1.0f, -1.0f);
	pRoot->localTransform.rotation = Quaternion::CreateFromYawPitchRoll(ToRadians(180.0f), 0.0f, 0.0f);

	m_skinMeshComp = AddComponent<MiniEngine::SkeletalMeshComponent>();
	PathManager* pathMgr = PathManager::GetInstance();

	std::wstring miniPath = pathMgr->ResolveAssetPath(L"Character.mini");
	std::shared_ptr<MiniEngine::SkinnedMesh> skinnedMesh = AssetManager::GetInstance()->LoadSkinnedMesh(miniPath);

	std::shared_ptr<SkeletalMeshComponent> skinComp = GetSkin().lock();
	skinComp->SetMesh(skinnedMesh);
	skinComp->AttachTo(GetRoot());

	InitAnimation(skinComp);

	{
		// 캐릭터 카메라 설정
		std::shared_ptr<MiniEngine::SceneComponent> pCamHolder = AddComponent<MiniEngine::SceneComponent>();
		pCamHolder->AttachTo(GetRoot());
		pCamHolder->localTransform.position = Vector3(0.0f, 1.5f, 0.0f);

		std::shared_ptr<CameraComponent> pCamComp = AddComponent<CameraComponent>();
		pCamComp->RegisterMainCamera();

		pCamComp->AttachTo(pCamHolder);
		pCamComp->localTransform.position = Vector3(0.0f, 0.0f, 4.0f);
		pCamComp->localTransform.rotation = Quaternion::CreateFromYawPitchRoll(ToRadians(180.0f), 0.0f, 0.0f);

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
		pCharCont->SetFallingSecondThreshold(0.3f); // 낙하 인정 시간 0.3초
		// pCharCont->SetLayerCollisionEnabled(MiniEngine::Physics::Layer::Obstacle, false);

		m_charCont = pCharCont;
	}

	{
		std::shared_ptr<PerceptionComponent> pPerceptComp = AddComponent<PerceptionComponent>();
		m_perception = pPerceptComp;
	}
}

void Character::InitAnimation(std::shared_ptr<SkeletalMeshComponent>& _skinComp)
{
	// 애니메이션 설정
	std::shared_ptr<Animator> pAnim = _skinComp->GetAnim().lock();
	std::shared_ptr<MiniEngine::SkinnedMesh> skinnedMesh = _skinComp->GetMesh().lock();
	
	pAnim->ReserveBaseLocomotion(static_cast<uint8_t>(EState::END));

	// 로코모션 구현
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
	}

	// ActionClip 구성
	{
		std::shared_ptr<ActionClip> pActionClip = std::make_shared<ActionClip>();
		pActionClip->AddClip(skinnedMesh->GetClipPtr(10));
		pActionClip->SetApplyRootBone(false); // jump는 루트모션 적용하지 않음
		
		std::shared_ptr<AnimNotify> pJump = std::make_shared<AnimNotify>(0.5f, 
			[this]() { GetController().lock()->Jump(m_jumpSpeed); });
		pActionClip->AddNotify(pJump);

		m_mapActions[(uint8_t)Content::Config::ETagAct::Jump] = pActionClip;
	}
	{
		std::shared_ptr<ActionClip> pActionClip = std::make_shared<ActionClip>();

		std::shared_ptr<AnimNotify> pIgnoreObstacle = std::make_shared<AnimNotify>(0.2f,
			[this]() { SetEnableCollisionObstacle(false); }
		);
		pActionClip->AddNotify(std::static_pointer_cast<IAnimNotify>(pIgnoreObstacle));

		std::shared_ptr<AnimNotify> pCollideObstacle = std::make_shared<AnimNotify>(0.7f,
			[this]() { SetEnableCollisionObstacle(true); }
		);
		pActionClip->AddNotify(std::static_pointer_cast<IAnimNotify>(pCollideObstacle));

		pActionClip->AddClip(skinnedMesh->GetClipPtr(11));
		m_mapActions[(uint8_t)Content::Config::ETagAct::Vault_Low] = pActionClip;
		m_mapActions[(uint8_t)Content::Config::ETagAct::Vault_Mid] = pActionClip;
		m_mapActions[(uint8_t)Content::Config::ETagAct::Mantle_Low] = pActionClip;
	}
	{
		std::shared_ptr<ActionClip> pActionClip = std::make_shared<ActionClip>();
		pActionClip->AddClip(skinnedMesh->GetClipPtr(12));
		// m_mapActions[(uint8_t)Content::Config::ETagAct::Mantle_Low] = pActionClip;
		m_mapActions[(uint8_t)Content::Config::ETagAct::Mantle_Mid] = pActionClip;
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

	InitInput();
}

void Character::Tick(float _dt)
{
	ProcessInput(_dt);
	
	Actor::Tick(_dt);

	// 판단 필요
	// 판단 내용 적용은 다음 tick에서 반영
	CheckCharacterState();

	/*float verticalVel = m_charCont.lock()->GetVerticalVelocity();
	if (verticalVel < 0.0f)
		MG_LOG_INFO("Character Fall {}", verticalVel);*/
}

void Character::ProcessInput(float _dt)
{
	// 임시 카메라 제어
	Input& input = InputManager::GetInstance()->GetInput();

	// 마우스 델타에 이미 델타타임이 곱해져 있음
	const Vector2 CamRotSpeed = m_camRotateSpeed * input.GetMouseDelta();
	m_camRotate.x += CamRotSpeed.x;
	m_camRotate.y += CamRotSpeed.y;
	m_camRotate.y = std::clamp(m_camRotate.y, -m_camMaxPitchDeg, m_camMaxPitchDeg); // 회전각 제한

	Quaternion qYaw = Quaternion::CreateFromAxisAngle(Vector3::Transform(Vector3(.0f, 1.0f, .0f), Quaternion(0.0f, 0.0f, 0.0f, 1.0f)), m_camRotate.x);
	Quaternion qPitch = Quaternion::CreateFromAxisAngle(Vector3(1.0f, 0.0f, 0.0f), m_camRotate.y);
	qYaw.Normalize();
	qPitch.Normalize();
	GetRoot()->localTransform.rotation = qYaw;

	std::shared_ptr<SceneComponent> pCamHolderRoot = m_cameraHolder.lock();
	pCamHolderRoot->localTransform.rotation = qPitch;
	
	// 키보드 이동키
	if (GetAnim().lock()->IsActionClipPlaying())
		return;

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
}

void Character::ProcessPerceptionResult()
{
	TravelResult result = m_perception.lock()->Travel();

	std::shared_ptr<ActionClip> pAction = GetActions(result.m_actTag);

	if (pAction)
		GetAnim().lock()->PlayActionClip(pAction, 0.2f);
}

void Character::CheckCharacterState()
{
	std::shared_ptr<Animator> pAnim = GetAnim().lock();
	std::shared_ptr<CharacterControllerComponent> pCharCont = m_charCont.lock();

	if (!pAnim || !pCharCont)
		return;
	
	// 공중인지 판단
	const bool bIsGrounded = pCharCont->IsGrounded();	// 땅에 닿았는지
	const bool bIsFalling = pCharCont->IsFalling();		// 실질적으로 떨어지고 있는지
	if (bIsFalling)
	{
		if (m_state == EState::InAir)
			return;

		m_state = EState::InAir;
		pAnim->TranstionBaseTrack(static_cast<uint8_t>(m_state), 0.25f);

		return;
	}

	if (bIsGrounded) 
	{
		if (m_state == EState::Landing)
			return;

		m_state = EState::Landing;
		pAnim->TranstionBaseTrack(static_cast<uint8_t>(m_state), 0.25f);

		return;
	}
	else
	{
		// 벽에 매달린 상황일 때
	}

}

void Character::SetInputDir(const Vector2& _dir)
{
	m_inputDir = _dir;
}

std::weak_ptr<Animator> Character::GetAnim() const
{
	return m_skinMeshComp.lock()->GetAnim();
}

void Character::SetEnableCollisionObstacle(bool _bEnable)
{
	GetController().lock()->SetLayerCollisionEnabled(MiniEngine::Physics::Layer::Obstacle, _bEnable);

	if(_bEnable)
		MG_LOG_INFO("Collide With Obstacle");
	else
		MG_LOG_INFO("Ignore Obstacle");
}

void Character::InitInput()
{
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
			std::shared_ptr<ActionClip> pJump = GetActions((uint8_t)Content::Config::ETagAct::Jump);
			GetAnim().lock()->PlayActionClip(pJump, 0.3f);
		});

	input.GetKeyBind(DirectX::Keyboard::Keys::LeftShift).OnPressed = std::bind(
		[this]() { ProcessPerceptionResult(); }
	);

}