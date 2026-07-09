#include "pch.h"
#include "Character.h"
#include "Core/Math.h"
#include "Platform/Input.h"
#include "Manager/AssetManager.h"
#include "Manager/PathManager.h"

#include "Scene/Scene.h"
#include "Scene/CameraComponent.h"
#include "Scene/RigidBodyComponent.h"
#include "Scene/SkeletalMeshComponent.h"
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
	m_skinMeshComp = AddComponent<MiniEngine::SkeletalMeshComponent>();
	// m_rigidBodyComp = AddComponent<MiniEngine::RigidBodyComponent>();

	PathManager* pathMgr = PathManager::GetInstance();

	std::wstring miniPath = pathMgr->ResolveAssetPath(L"YBot.mini");
	std::shared_ptr<MiniEngine::SkinnedMesh> skinnedMesh = AssetManager::GetInstance()->LoadSkinnedMesh(miniPath);

	std::shared_ptr<SkeletalMeshComponent> skinComp = GetSkin().lock();
	skinComp->SetMesh(skinnedMesh);

	std::shared_ptr<SceneComponent> charRoot = GetRoot();
	charRoot->localTransform.position = Vector3(0.0f, 0.0f, -1.0f);
	charRoot->localTransform.rotation = Quaternion::CreateFromYawPitchRoll(ToRadians(180.0f), 0.0f, 0.0f);

	{
		// 애니메이션 설정
		std::shared_ptr<Animator> pAnim = skinComp->GetAnim().lock();
		// 로코모션 구현
		m_tempLoco = std::make_shared<BlendClip>(5);
		// 모션 입력 
		// TODO : Editor에서 좀 사용하기 쉽게 개선해야함
		m_tempLoco->AddAnimClip({ 0, 0 }, skinnedMesh->GetClipPtr(1));	// idle
		m_tempLoco->AddAnimClip({ 0, 1 }, skinnedMesh->GetClipPtr(2));	// Walking
		m_tempLoco->AddAnimClip({ 0, -1 }, skinnedMesh->GetClipPtr(5));	// Walking Backword
		m_tempLoco->AddAnimClip({ 1, 0 }, skinnedMesh->GetClipPtr(3));	// right strafe
		m_tempLoco->AddAnimClip({ -1, 0 }, skinnedMesh->GetClipPtr(4));	// left strafe

		std::shared_ptr<IAnimatorClip> pLoco = std::dynamic_pointer_cast<IAnimatorClip>(m_tempLoco);
		pAnim->AddLocomotion(pLoco);

		m_tempActionClip = std::make_shared<ActionClip>();
		m_tempActionClip->AddClip(skinnedMesh->GetClipPtr(6));
	}

	{
		// 캐릭터 카메라 설정
		std::shared_ptr<CameraComponent> pCamComp = AddComponent<CameraComponent>();
		pCamComp->aspect = static_cast<float>(WINDOW_WIDTH) / static_cast<float>(WINDOW_HEIGHT);
		pCamComp->RegisterMainCamera();

		pCamComp->AttachTo(GetRoot());
		pCamComp->localTransform.position = Vector3(0.0f, 1.5f, 3.0f);
		pCamComp->localTransform.rotation = Quaternion::CreateFromYawPitchRoll(ToRadians(180.0f), 0.0f, 0.0f);
	}

	{
		// RigidBody 설정
		// Vector2 capsuleExtent(1.0f, 1.8f);
		// m_rigidBodyComp.lock()->InitDynamicCapsule(*GetScene()->GetPhysics().lock(), capsuleExtent * 0.5f);
	}
}

void Character::BeginPlay()
{
	Actor::BeginPlay();

	InitInput();
}

void Character::Tick(float _dt)
{
	Actor::Tick(_dt);

	ProcessInput(_dt);
}

void Character::ProcessInput(float _dt)
{

	if (m_inputDir.LengthSquared() > 0)
	{
		// 임시 이동 코드
		const float deltaSpeed = _dt * m_moveSpeed;
		std::shared_ptr<SceneComponent> root = GetRoot();

		// 캐릭터 정면 기준 이동
		const Vector3& fwd = root->localTransform.GetMatrix().Forward();
		const Vector3& rht = root->localTransform.GetMatrix().Right();

		root->localTransform.position +=
			deltaSpeed * m_inputDir.y * fwd +
			deltaSpeed * m_inputDir.x * rht;
	}
}

void Character::SetInputDir(const Vector2& _dir)
{
	m_inputDir = _dir;

	if (!m_tempLoco)
		return;

	m_tempLoco->SetAxisValue(m_inputDir.x, m_inputDir.y);
}

std::weak_ptr<Animator> Character::GetAnim() const
{
	return m_skinMeshComp.lock()->GetAnim();
}

void Character::InitInput()
{
	// 바인딩 하드코딩
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
			inputDir.x = -1.0f;
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
			inputDir.x = 1.0f;
			SetInputDir(inputDir);
		});
	input.GetKeyBind(DirectX::Keyboard::Keys::Left).OnReleased = std::bind(
		[this]()
		{
			Vector2 inputDir = GetInputDir();
			inputDir.x = 0.0f;
			SetInputDir(inputDir);
		});

	input.GetKeyBind(DirectX::Keyboard::Keys::Space).OnReleased = std::bind(
		[this]()
		{
			GetAnim().lock()->PlayActionClip(m_tempActionClip, 0.5f);
		});

}

void Character::TestRaycast()
{
	std::shared_ptr<Physics::PhysicsWorld> pPhysics = GetScene()->GetPhysics().lock();
	
	Transform& localTrs = GetRoot()->localTransform;

	Physics::RaycastParam rayParam;
	rayParam.m_origin = localTrs.position;
	rayParam.m_dir = localTrs.Forward();
	rayParam.m_maxDistance = 1.0f;

	Physics::RaycastResult hitResult;
	if (!pPhysics->Raycast(rayParam, hitResult))
		return;

	void* pActor = hitResult.GetActor();
}
