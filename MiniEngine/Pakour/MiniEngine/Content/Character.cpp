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

	std::wstring miniPath = pathMgr->ResolveAssetPath(L"YBot.mini");
	std::shared_ptr<MiniEngine::SkinnedMesh> skinnedMesh = AssetManager::GetInstance()->LoadSkinnedMesh(miniPath);

	std::shared_ptr<SkeletalMeshComponent> skinComp = GetSkin().lock();
	skinComp->SetMesh(skinnedMesh);
	skinComp->AttachTo(GetRoot());

	{
		// 애니메이션 설정
		std::shared_ptr<Animator> pAnim = skinComp->GetAnim().lock();
		// 로코모션 구현
		std::shared_ptr<BlendClip> pBlend = std::make_shared<BlendClip>(9);
		// 모션 입력 
		// TODO : Editor에서 좀 사용하기 쉽게 개선해야함
		pBlend->AddAnimClip({ 0, 0 },		skinnedMesh->GetClipPtr(1));	// idle
		pBlend->AddAnimClip({ 0, 1 },		skinnedMesh->GetClipPtr(6));	// Walking
		pBlend->AddAnimClip({ 0.5, 1 },		skinnedMesh->GetClipPtr(6));	// Walking
		pBlend->AddAnimClip({ -0.5, 1 },	skinnedMesh->GetClipPtr(6));	// Walking
		pBlend->AddAnimClip({ 0, -1 },		skinnedMesh->GetClipPtr(9));	// Walking Backword
		pBlend->AddAnimClip({ 0.5, -1 },	skinnedMesh->GetClipPtr(9));	// Walking Backword
		pBlend->AddAnimClip({ -0.5, -1 },	skinnedMesh->GetClipPtr(9));	// Walking Backword
		pBlend->AddAnimClip({ 1, 0 },		skinnedMesh->GetClipPtr(8));	// right strafe
		pBlend->AddAnimClip({ -1, 0 },		skinnedMesh->GetClipPtr(7));	// left strafe

		pAnim->ReserveBaseLocomotion(1);
		pAnim->AddBaseLocomotion(pBlend);

		// 단일 재생
		m_mapActions[(uint8_t)Content::Config::ETagAct::Jump] = std::make_shared<ActionClip>();
		m_mapActions[(uint8_t)Content::Config::ETagAct::Jump]->AddClip(skinnedMesh->GetClipPtr(10));
		m_mapActions[(uint8_t)Content::Config::ETagAct::Vault] = std::make_shared<ActionClip>();
		m_mapActions[(uint8_t)Content::Config::ETagAct::Vault]->AddClip(skinnedMesh->GetClipPtr(11));

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
		pCharCont->SetLayer(MiniEngine::Physics::Layer::Character);

		// uint32_t CollisionMasks = MiniEngine::Physics::LayerMask::ALL | ~static_cast<uint32_t>(MiniEngine::Physics::Layer::Obstacle);
		pCharCont->SetCollisionMask(MiniEngine::Physics::LayerMask::ALL);

		m_charCont = pCharCont;
	}

	{
		std::shared_ptr<PerceptionComponent> pPerceptComp = AddComponent<PerceptionComponent>();
		m_perception = pPerceptComp;
	}
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
}

void Character::ProcessInput(float _dt)
{
	// 임시 카메라 제어
	if (m_camRotDir.LengthSquared() > 0)
	{
		const float deltaSpeed = _dt * m_camRotateSpeed;
		std::shared_ptr<SceneComponent> pRoot = m_cameraHolder.lock();

		Quaternion rotDt = Quaternion::CreateFromAxisAngle(Vector3(0.0f, 1.0f, 0.0f), deltaSpeed * m_camRotDir.x); 
		/* *
			Quaternion::CreateFromAxisAngle(Vector3(1.0f, 0.0f, 0.0f), deltaSpeed * m_camRotDir.y)*/;
		// rotDt.z = 0.0f;
		rotDt.Normalize();

		pRoot->localTransform.rotation *= rotDt;
	}

	// 키보드 이동키
	{
		if (GetAnim().lock()->IsActionClipPlaying())
			return;

		// 임시 이동 코드
		Vector2 input = m_inputDir;
		input.Normalize();

		m_lerpInputDir = Vector2::Lerp(m_lerpInputDir, input, m_lerpWeight * _dt);

		const float deltaSpeed = _dt * m_moveSpeed;
		std::shared_ptr<SceneComponent> pRoot = GetRoot();

		// 캐릭터 정면 기준 이동
		const Vector3& fwd = pRoot->localTransform.Forward();
		const Vector3& rht = pRoot->localTransform.Right();

		m_charCont.lock()->AddMovementInput(deltaSpeed * m_lerpInputDir.y * fwd + deltaSpeed * -m_lerpInputDir.x * rht);

		GetAnim().lock()->SetBaseTrackInputAxis(m_lerpInputDir);
	}
}

void Character::SetInputDir(const Vector2& _dir)
{
	m_inputDir = _dir;
}

void Character::SetCamRotDir(const Vector2& _dir)
{
	m_camRotDir = _dir;
}

std::weak_ptr<Animator> Character::GetAnim() const
{
	return m_skinMeshComp.lock()->GetAnim();
}

void Character::InitInput()
{
	// 바인딩
	Input& input = InputManager::GetInstance()->GetInput();

	input.GetKeyBind(DirectX::Keyboard::Keys::Escape).OnPressed = std::bind([this]() { PostQuitMessage(0); });
	input.GetKeyBind(DirectX::Keyboard::Keys::Up).OnPressed = std::bind(
		[this]()
		{
			Vector2 inputDir = GetInputDir();
			inputDir.y = 1.0f;
			SetInputDir(inputDir);
		});
	input.GetKeyBind(DirectX::Keyboard::Keys::Up).OnReleased = std::bind(
		[this]()
		{
			Vector2 inputDir = GetInputDir();
			inputDir.y = 0.0f;
			SetInputDir(inputDir);
		});

	input.GetKeyBind(DirectX::Keyboard::Keys::Down).OnPressed = std::bind(
		[this]()
		{
			Vector2 inputDir = GetInputDir();
			inputDir.y = -1.0f;
			SetInputDir(inputDir);
		});
	input.GetKeyBind(DirectX::Keyboard::Keys::Down).OnReleased = std::bind(
		[this]()
		{
			Vector2 inputDir = GetInputDir();
			inputDir.y = 0.0f;
			SetInputDir(inputDir);
		});

	input.GetKeyBind(DirectX::Keyboard::Keys::Right).OnPressed = std::bind(
		[this]()
		{
			Vector2 inputDir = GetInputDir();
			inputDir.x = 1.0f;
			SetInputDir(inputDir);
		});
	input.GetKeyBind(DirectX::Keyboard::Keys::Right).OnReleased = std::bind(
		[this]()
		{
			Vector2 inputDir = GetInputDir();
			inputDir.x = 0.0f;
			SetInputDir(inputDir);
		});

	input.GetKeyBind(DirectX::Keyboard::Keys::Left).OnPressed = std::bind(
		[this]()
		{
			Vector2 inputDir = GetInputDir();
			inputDir.x = -1.0f;
			SetInputDir(inputDir);
		});
	input.GetKeyBind(DirectX::Keyboard::Keys::Left).OnReleased = std::bind(
		[this]()
		{
			Vector2 inputDir = GetInputDir();
			inputDir.x = 0.0f;
			SetInputDir(inputDir);
		});

	input.GetKeyBind(DirectX::Keyboard::Keys::A).OnPressed = std::bind(
		[this]() 
		{
			Vector2 dir = GetCamRotDir();
			dir.x = -1.0f;
			SetCamRotDir(dir);
		});
	input.GetKeyBind(DirectX::Keyboard::Keys::A).OnReleased = std::bind(
		[this]()
		{
			Vector2 dir = GetCamRotDir();
			dir.x = 0.0f;
			SetCamRotDir(dir);
		});

	input.GetKeyBind(DirectX::Keyboard::Keys::D).OnPressed = std::bind(
		[this]()
		{
			Vector2 dir = GetCamRotDir();
			dir.x = 1.0f;
			SetCamRotDir(dir);
		});
	input.GetKeyBind(DirectX::Keyboard::Keys::D).OnReleased = std::bind(
		[this]()
		{
			Vector2 dir = GetCamRotDir();
			dir.x = 0.0f;
			SetCamRotDir(dir);
		});

	input.GetKeyBind(DirectX::Keyboard::Keys::W).OnPressed = std::bind(
		[this]()
		{
			Vector2 dir = GetCamRotDir();
			dir.y = -1.0f;
			SetCamRotDir(dir);
		});
	input.GetKeyBind(DirectX::Keyboard::Keys::W).OnReleased = std::bind(
		[this]()
		{
			Vector2 dir = GetCamRotDir();
			dir.y = 0.0f;
			SetCamRotDir(dir);
		});

	input.GetKeyBind(DirectX::Keyboard::Keys::S).OnPressed = std::bind(
		[this]()
		{
			Vector2 dir = GetCamRotDir();
			dir.y = 1.0f;
			SetCamRotDir(dir);
		});
	input.GetKeyBind(DirectX::Keyboard::Keys::S).OnReleased = std::bind(
		[this]()
		{
			Vector2 dir = GetCamRotDir();
			dir.y = 0.0f;
			SetCamRotDir(dir);
		});

	// 테스트용 점프
	input.GetKeyBind(DirectX::Keyboard::Keys::Space).OnReleased = std::bind(
		[this]()
		{
			std::shared_ptr<ActionClip> pJump = GetActions((uint8_t)Content::Config::ETagAct::Jump);
			GetAnim().lock()->PlayActionClip(pJump, 0.3f);

			m_charCont.lock()->Jump(m_jumpSpeed);
		});

	// 레이캐스트
	/*input.GetKeyBind(DirectX::Keyboard::Keys::LeftShift).Pressing = std::bind(
		[this](float _dt) 
		{
			TravelResult result = m_perception.lock()->Travel();

			if(result.m_actTag == static_cast<uint8_t>(Content::Config::ETagAct::Vault))
				MG_LOG_INFO("[Character] Play Valut!");
			else if (result.m_actTag == static_cast<uint8_t>(Content::Config::ETagAct::Mantle))
				MG_LOG_INFO("[Character] Play Mantle!");
			else
				MG_LOG_INFO("[Character] Can't Found!");

			std::shared_ptr<ActionClip> pAction = GetActions(result.m_actTag);
			if(pAction)
				GetAnim().lock()->PlayActionClip(pAction, 0.3f);

		}, std::placeholders::_1);
		*/

	input.GetKeyBind(DirectX::Keyboard::Keys::LeftShift).OnPressed = std::bind(
		[this]() 
		{
			TravelResult result = m_perception.lock()->Travel();
			
			if(result.m_actTag == static_cast<uint8_t>(Content::Config::ETagAct::Vault))
				MG_LOG_INFO("[Character] Play Valut!");
			else if (result.m_actTag == static_cast<uint8_t>(Content::Config::ETagAct::Mantle))
				MG_LOG_INFO("[Character] Play Mantle!");
			else
				MG_LOG_INFO("[Character] Can't Found!");

			std::shared_ptr<ActionClip> pAction = GetActions(result.m_actTag);
			if(pAction)
				GetAnim().lock()->PlayActionClip(pAction, 0.3f);
		}
	);

}