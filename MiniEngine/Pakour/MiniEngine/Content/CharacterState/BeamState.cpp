#include "pch.h"
#include "Content/CharacterState/BeamState.h"
#include "Scene/IObstacle.h"
#include "Content/Character.h"
#include "Platform/Input.h"

#include "Scene/Scene.h"
#include "Physics/PhysicsWorld.h"

#include "Core/Log.h"

using namespace Content::Config;

void BeamState::OnStart()
{
	std::shared_ptr<Character> pChar = GetMachine()->GetCharacter();
	pChar->TranstionBaseTrack((uint8_t)pChar->GetState());
	pChar->SetUseGravity(false);

	Refresh(); // 최초에도 한번 refresh 호출
}

void BeamState::Refresh()
{
	std::shared_ptr<Character> pChar = GetMachine()->GetCharacter();
	// Beam 상태 진입의 전제
	// Beam 태그가 달린 오브젝트를 처리하고 있을 것
	const Character::PerceptedObstacleInfo& OBS_INFO = pChar->GetCurObstacleInfo();
	assert(OBS_INFO.IsValid());

	uint8_t subInfoTag;
	if (OBS_INFO.m_pObstacle->TryGetTag(TAG_SUB_INFO, subInfoTag))
	{
		m_curAxis = (ETagAxis)subInfoTag;

		AlignByAxis();
	}
}

void BeamState::OnEnd()
{
	std::shared_ptr<Character> pChar = GetMachine()->GetCharacter();
	pChar->SetUseGravity(true);
}

void BeamState::Tick(float _dt)
{
	CheckState();
	// CameraRotate(_dt);
}
void BeamState::LateTick(float _dt){}


MiniEngine::IObstacle* BeamState::GetCurObs() const
{
	std::shared_ptr<Character> pChar = GetMachine()->GetCharacter();
	return pChar->GetCurObstacleInfo().m_pObstacle;
}

bool BeamState::ObstacleIsBeamType(Actor* _pObs)
{
	uint8_t typeTag;

	if (_pObs->GetTag().GetTagAt(TAG_ENV_DETAIL, typeTag))
		return typeTag == (uint8_t)ETagEnvDetail::Beam;

	return false;
}

void BeamState::GetDirectionByAxis(Vector3& _outDir)
{
	switch (GetAxis())
	{
	case ETagAxis::X:
		_outDir = GetCurObs()->GetTransform().Right();
		break;
	// case ETagAxis::Y : y 방향으로 긴 경우 -> 이건 기둥 형태라 생각하고 배제
	case ETagAxis::Z:
		_outDir = GetCurObs()->GetTransform().Forward();
		break;
	}
}

void BeamState::AdjustPositionToObstacleInfo()
{
	std::shared_ptr<Character> pChar = GetMachine()->GetCharacter();
	Character::PerceptedObstacleInfo& obsInfo = pChar->GetCurObstacleInfo();
	
	const Vector3& POS = pChar->GetRoot()->localTransform.position;
	obsInfo.m_obstacleHitPos.x = POS.x;
	obsInfo.m_obstacleHitPos.z = POS.z;
}


// Beam Stand
void BeamStandState::Tick(float _dt)
{
	BeamState::Tick(_dt);

	ProcessMovement(_dt);
}

void BeamStandState::AlignByAxis()
{
	// 좁은 발판에 선 상황
	const IObstacle* pCurObs = GetCurObs();
	if (!pCurObs)
		return;

	std::shared_ptr<Character> pChar = GetMachine()->GetCharacter();
	Transform& charTF = pChar->GetRoot()->localTransform;

	Vector3 obsDir; // Beam 지형의 주 방향 (긴쪽 방향)
	GetDirectionByAxis(obsDir);
	obsDir.y = 0.0f;
	obsDir.Normalize();
	Vector3 obsLeftAngled{ -obsDir.z, obsDir.y, obsDir.x }; // Beam 지형의 넓은 쪽 방향 (긴쪽의 90도 왼쪽 회전)

	const float DOT_DIR = obsDir.Dot(charTF.Forward());
	const float DOT_LEFT_DIR = obsLeftAngled.Dot(charTF.Forward());

	Vector3 toward =
		abs(DOT_DIR) >= cosf(ToRadians(45.0f)) ?
		(DOT_DIR >= 0.0f ? obsDir : -obsDir) :
		(DOT_LEFT_DIR >= 0.0f ? obsLeftAngled : -obsLeftAngled);

	Quaternion rot;
	if (TryYawRotateToward(toward, rot))
		charTF.rotation = rot;
}

bool BeamStandState::IsAlignToAxis()
{
	// 캐릭터의 정면이 봉의 축과 일치하는지 확인
	Vector3 obsDir; // Beam 지형의 주 방향 (긴쪽 방향)
	GetDirectionByAxis(obsDir);
	obsDir.y = 0.0f;
	obsDir.Normalize();
	Vector3 obsLeftAngled{ -obsDir.z, obsDir.y, obsDir.x };

	std::shared_ptr<Character> pChar = GetMachine()->GetCharacter();
	Transform& charTF = pChar->GetRoot()->localTransform;

	const float DOT_DIR = obsDir.Dot(charTF.Forward());

	return fabs(DOT_DIR) >= 0.95f;
}

void BeamStandState::ProcessMovement(float _dt)
{
	std::shared_ptr<Character> pChar = GetMachine()->GetCharacter();
	if (pChar->IsActionClipPlaying())
		return;

	const Vector2& INPUT_DIR = pChar->GetInputDir();

	if (INPUT_DIR.x < 0)
	{
		if (std::shared_ptr<ActionClip> pAct = pChar->GetActions((uint8_t)ETagAct::Beam_StandRotateLeft))
			pChar->PlayActionClip(pAct, 0.1f);
		return;
	}
	else if (INPUT_DIR.x > 0) 
	{
		if (std::shared_ptr<ActionClip> pAct = pChar->GetActions((uint8_t)ETagAct::Beam_StandRotateRight))
			pChar->PlayActionClip(pAct, 0.1f);
		return;
	}
	else if (INPUT_DIR.y < 0) 
	{
		if (IsAlignToAxis())
			return;

		// Beam 위에서 움직였다면 그만큼 위치를 변경
		AdjustPositionToObstacleInfo();
		AdjustRotationToObstacleInfo();

		if (std::shared_ptr<ActionClip> pAct = pChar->GetActions((uint8_t)ETagAct::Beam_StandMoveDown))
			pChar->PlayActionClip(pAct, 0.2f);

		return;
	}
	
	Vector2& inputLerp = pChar->InputLerp();
	inputLerp = Vector2::Lerp(inputLerp, INPUT_DIR, pChar->GetInputLerpWeight());
	
	// 가지 못하는 상황
	if (CheckEnableToMove(pChar) == false ||
		inputLerp.y < 0)
	{
		inputLerp.y = 0.0f;
		pChar->SetAnimBaseTrackInputAxis(inputLerp);
		return;
	}

	// y축 입력 시, 앞 뒤로 이동
	// 후방 이동은 모션이 없어 제외 -> 블렌드 시, idle이 최종임
	pChar->SetAnimBaseTrackInputAxis(inputLerp);

	const float DELTA_SPD = _dt * pChar->GetMoveSpeed();
	const Transform& TF = pChar->GetRoot()->localTransform;
	pChar->AddMovementInput(DELTA_SPD * inputLerp.y * TF.Forward());
}

bool BeamStandState::CheckEnableToMove(std::shared_ptr<Character>& _pChar)
{
	// 정면 바로 아래가 절벽인지 확인
	const Transform& TF = _pChar->GetRoot()->localTransform;

	MiniEngine::Physics::SpherecastParam param;
	param.m_dir = Vector3(0.0f, -1.0f, 0.0f);
	param.m_maxDistance = 0.5f;
	param.m_startPos = TF.position + TF.Forward() * 0.3f + Vector3(0.0f, 0.3f, 0.0f);
	param.m_radius = 0.3f;

	MiniEngine::Physics::RaycastResult result;
	bool bIsHit = _pChar->GetScene()->GetPhysics().lock()->SphereCast(param, result, MiniEngine::Physics::ToMask(MiniEngine::Physics::Layer::Obstacle));

	if (!bIsHit)
		return false;

	Actor* pActor = reinterpret_cast<Actor*>(result.GetActor());
	IObstacle* pObs = dynamic_cast<IObstacle*>(pActor);

	return pObs == GetCurObs();
}

void BeamStandState::AdjustRotationToObstacleInfo()
{
	std::shared_ptr<Character> pChar = GetMachine()->GetCharacter();
	Character::PerceptedObstacleInfo& obsInfo = pChar->GetCurObstacleInfo();

	Vector3 fwd = pChar->GetRoot()->localTransform.Forward();
	float d = fwd.Dot(obsInfo.m_obstacleHitNrm);
	if (d > 0.0f && std::fabs(d) > 0.95f)
	{
		obsInfo.m_obstacleHitNrm *= -1.0f;
	}
}

void BeamHangingState::OnStart()
{
	BeamState::OnStart();

	std::shared_ptr<Character> pChar = GetMachine()->GetCharacter();
	pChar->ReserveIKDetectBeamHanging();
}

void BeamHangingState::OnEnd()
{
	BeamState::OnEnd();

	std::shared_ptr<Character> pChar = GetMachine()->GetCharacter();
	pChar->ClearIKReserve();
}

// Beam Hanging
void BeamHangingState::Tick(float _dt)
{
	BeamState::Tick(_dt);

	ProcessMovement(_dt);
}

void BeamHangingState::AlignByAxis()
{
	// 봉에 매달린 상황
	const IObstacle* pCurObs = GetCurObs();
	if (!pCurObs)
		return;

	std::shared_ptr<Character> pChar = GetMachine()->GetCharacter();
	Transform& charTF = pChar->GetRoot()->localTransform;

	// 봉은 긴쪽을 기준으로만 매달리기 -> 둘다 해봤을 때, 이 방식이 그나마 자연스러움
	Vector3 obsDir; // Beam 지형의 주 방향 (긴쪽 방향)
	GetDirectionByAxis(obsDir);
	obsDir.y = 0.0f;
	obsDir.Normalize();
	obsDir = { -obsDir.z, obsDir.y, obsDir.x };

	const float DOT_DIR = obsDir.Dot(charTF.Forward());
	Vector3 toward = DOT_DIR > 0.0f ? obsDir : -obsDir;
	
	Quaternion rot;
	if (TryYawRotateToward(toward, rot))
		charTF.rotation = rot;

}

void BeamHangingState::ProcessMovement(float _dt)
{
	// 봉의 방향에 따라 좌우로만 이동
	std::shared_ptr<Character> pChar = GetMachine()->GetCharacter();
	if (pChar->IsActionClipPlaying())
		return;

	const Vector2 INPUT_DIR = pChar->GetInputDir();

	ETagAct eAct = ETagAct::End;
	if (INPUT_DIR.x > 0 && CheckEnableToMove(pChar, INPUT_DIR))
		eAct = ETagAct::Beam_HangingMoveRight;
	else if (INPUT_DIR.x < 0 && CheckEnableToMove(pChar, INPUT_DIR))
		eAct = ETagAct::Beam_HangingMoveLeft;
	else if (INPUT_DIR.y > 0)
	{
		AdjustPositionToObstacleInfo(); // Beam 위에서 움직였다면 그만큼 위치를 변경
		eAct = ETagAct::Beam_HangingMoveUp;
	}
	else if (INPUT_DIR.y < 0)
	{
		AdjustPositionToObstacleInfo(); // Beam 위에서 움직였다면 그만큼 위치를 변경
		eAct = ETagAct::Beam_HangingMoveDown;
	}

	if (std::shared_ptr<ActionClip> pAct = pChar->GetActions((uint8_t)eAct))
		pChar->PlayActionClip(pAct, 0.2f);
}

bool BeamHangingState::CheckEnableToMove(std::shared_ptr<Character>& _pChar, const Vector2& _inputDir)
{
	// 좌우 방향 이동 시, 위쪽에 철봉이 계속되는지 확인
	const Transform& TF = _pChar->GetRoot()->localTransform;

	MiniEngine::Physics::SpherecastParam param;
	param.m_radius = 0.3f;
	param.m_dir = Vector3(0.f, 1.0f, 0.f);
	param.m_maxDistance = 0.5f;
	param.m_startPos = TF.position + Vector3(0.0f, _pChar->GetCharacterHalfHeight() * 2.0f, 0.0f) + _inputDir.x * TF.Right();

	MiniEngine::Physics::RaycastResult result;
	bool bIsHit = _pChar->GetScene()->GetPhysics().lock()->SphereCast(param, result, MiniEngine::Physics::ToMask(MiniEngine::Physics::Layer::Obstacle));

	if (!bIsHit)
		return false;

	Actor* pActor =  reinterpret_cast<Actor*>(result.GetActor());
	IObstacle* pObs = dynamic_cast<IObstacle*>(pActor);

	return pObs == GetCurObs();
}

