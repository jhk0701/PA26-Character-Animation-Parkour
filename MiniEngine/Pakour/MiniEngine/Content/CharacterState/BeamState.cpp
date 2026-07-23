#include "pch.h"
#include "Content/CharacterState/BeamState.h"
#include "Content/Character.h"
#include "Platform/Input.h"

#include "Scene/Scene.h"
#include "Physics/PhysicsWorld.h"

#include "Core/Log.h"

using namespace Content::Config;

void BeamState::OnStart()
{
	CameraFixedState::OnStart();

	// Beam 장애물에 snap된 상태
	std::shared_ptr<Character> pChar = GetMachine()->GetCharacter();
	pChar->TranstionBaseTrack((uint8_t)pChar->GetState());

	MG_LOG_INFO("[Character] Beam State Started");
	pChar->SetUseGravity(false);

	Refresh();
}

void BeamState::Refresh()
{
	std::shared_ptr<Character> pChar = GetMachine()->GetCharacter();

	// Beam 상태 진입의 전제
	// Beam 태그가 달린 오브젝트를 처리하고 있을 것
	const Character::PerceptedObstacleInfo& OBS_INFO = pChar->GetCurObstacleInfo();
	assert(OBS_INFO.IsValid());

	uint8_t subInfoTag;
	OBS_INFO.m_pObstacle->GetTag().GetTagAt(TAG_SUB_INFO, subInfoTag);

	m_curAxis = (ETagAxis)subInfoTag;
	m_pCurObs = OBS_INFO.m_pObstacle;

	OrientByAxis();
}

void BeamState::OnEnd()
{
	CameraFixedState::OnEnd();

	std::shared_ptr<Character> pChar = GetMachine()->GetCharacter();

	pChar->SetUseGravity(true);

	m_pCurObs = nullptr;
}

void BeamState::Tick(float _dt)
{
	CheckState();
	CameraRotate(_dt);
}

void BeamState::CheckState()
{
	CharacterState::CheckState();
}
void BeamState::ProcessPerceptionResult(const Character::PerceptedObstacleInfo& _info) {}

bool BeamState::ObstacleIsBeamType(Actor* _pObs)
{
	uint8_t typeTag;

	if (_pObs->GetTag().GetTagAt(TAG_ENV_DETAIL, typeTag))
		return typeTag == (uint8_t)ETagEnvDetail::Beam;

	return false;
}

Vector3 BeamState::GetDirectionByAxis()
{
	Vector3 dir;

	switch (GetAxis())
	{
	case ETagAxis::X:
		dir = GetCurObs()->GetRoot()->localTransform.Right();
		break;
	case ETagAxis::Y:
		dir = GetCurObs()->GetRoot()->localTransform.Up();
		break;
	case ETagAxis::Z:
		dir = GetCurObs()->GetRoot()->localTransform.Forward();
		break;
	}

	return dir;
}

// Beam Stand
void BeamStandState::OnStart()
{
	BeamState::OnStart();
}

void BeamStandState::OnEnd()
{
	BeamState::OnEnd();
}

void BeamStandState::Tick(float _dt)
{
	BeamState::Tick(_dt);

	// DefaultCameraRotate(_dt);
	ProcessMovement(_dt);
	CheckState();
}

void BeamStandState::CheckState()
{
	BeamState::CheckState();

	// 해결후 테스트 확인
	std::shared_ptr<Character> pChar = GetMachine()->GetCharacter();
	
	// 현재 올라온 장애물 상태 확인
	const Actor* pCurObs = GetCurObs();
	if (!pCurObs || (pChar->GetCurObstacleInfo().IsValid() && 
		!ObstacleIsBeamType(pChar->GetCurObstacleInfo().m_pObstacle)))
	{
		// 랜딩으로 전환

		pChar->SetState(Character::EState::Landing);
		const uint8_t STATE = (uint8_t)pChar->GetState();;
		GetMachine()->Transition(STATE);
	}
}

void BeamStandState::ProcessPerceptionResult(const Character::PerceptedObstacleInfo& _info)
{
	BeamState::ProcessPerceptionResult(_info);
	DefaultProcessPerceptionResult(_info);
}

void BeamStandState::OrientByAxis()
{
	// 회전각 제한은 우선 보류
	return;

	// 좁은 발판에 선 상황
	const Actor* pCurObs = GetCurObs();
	if (!pCurObs)
		return;

	std::shared_ptr<Character> pChar = GetMachine()->GetCharacter();
	Transform& charTF = pChar->GetRoot()->localTransform;
	
	Vector3 obsDir, obsLeftAngled;
	obsDir = GetDirectionByAxis();

	// 발판을 전제로 함
	// y 방향으로 향하는 경우는 우선 배제
	obsDir.y = 0.0f;
	obsDir.Normalize();
	obsLeftAngled = Vector3(0.0f, 1.0f, 0.0f).Cross(obsDir);
	obsLeftAngled.Normalize();

	const Vector3 CHAR_FWD = charTF.Forward();
	
	const float OBS_AXIS_DOT = CHAR_FWD.Dot(obsDir);
	const float OBS_LEFT_DOT = CHAR_FWD.Dot(obsLeftAngled);
	
	bool bAlignToAxis = fabs(OBS_AXIS_DOT) >= cosf(ToRadians(45.0f));
	Vector3 toward = bAlignToAxis ? obsDir : obsLeftAngled;

	if (bAlignToAxis == false && OBS_LEFT_DOT < 0)
		toward *= -1;
	
	charTF.rotation = Quaternion::LookRotation(toward, Vector3(0.0f, 1.0f, 0.0f));
}

void BeamStandState::ProcessMovement(float _dt)
{
	std::shared_ptr<Character> pChar = GetMachine()->GetCharacter();
	if (pChar->IsActionClipPlaying())
		return;

	const Vector2& INPUT_DIR = pChar->GetInputDir();

	// x 축 입력 시 회전
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
	
	Vector2& inputLerp = pChar->InputLerp();
	inputLerp = Vector2::Lerp(inputLerp, INPUT_DIR, pChar->GetInputLerpWeight());
	
	// 가지 못하는 상황
	/*IsAlignToAxis(pChar) == false ||*/
	if (CheckEnableToMove() == false ||
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


bool BeamStandState::IsAlignToAxis(std::shared_ptr<Character> _pChar)
{
	Vector3 dir = GetDirectionByAxis();
	dir.y = 0.0f;
	dir.Normalize();

	float dot = fabs(dir.Dot(_pChar->GetRoot()->localTransform.Forward()));
	return dot >= 0.90f;
}

bool BeamStandState::CheckEnableToMove()
{
	// 정면 바로 아래가 절벽인지 확인
	std::shared_ptr<Character> pChar = GetMachine()->GetCharacter();
	const Transform& TF = pChar->GetRoot()->localTransform;

	MiniEngine::Physics::SpherecastParam param;
	param.m_dir = Vector3(0.0f, -1.0f, 0.0f);
	param.m_maxDistance = 0.5f;
	param.m_startPos = TF.position + TF.Forward() * 0.3f + Vector3(0.0f, 0.3f, 0.0f);
	param.m_radius = 0.3f;

	MiniEngine::Physics::RaycastResult result;
	bool bIsHit = pChar->GetScene()->GetPhysics().lock()->SphereCast(param, result, MiniEngine::Physics::ToMask(MiniEngine::Physics::Layer::Obstacle));
	if (bIsHit && result.GetActor() == GetCurObs())
		return true;

	return false;
}

// Beam Hanging
void BeamHangingState::OnStart()
{
	BeamState::OnStart();
}

void BeamHangingState::OnEnd()
{
	BeamState::OnEnd();
}

void BeamHangingState::Tick(float _dt)
{
	BeamState::Tick(_dt);

	ProcessMovement(_dt);
}

void BeamHangingState::CheckState()
{
	BeamState::CheckState();
}

void BeamHangingState::ProcessPerceptionResult(const Character::PerceptedObstacleInfo& _info)
{
	BeamState::ProcessPerceptionResult(_info);
}

void BeamHangingState::OrientByAxis()
{
}

void BeamHangingState::ProcessMovement(float _dt)
{
	// 봉의 방향에 따라 좌우로만 이동
	std::shared_ptr<Character> pChar = GetMachine()->GetCharacter();
	if (pChar->IsActionClipPlaying())
		return;

	const Vector2 INPUT_DIR = pChar->GetInputDir();

	ETagAct eAct = ETagAct::End;
	if (INPUT_DIR.x > 0)
		eAct = ETagAct::Beam_HangingMoveRight;
	else if (INPUT_DIR.x < 0)
		eAct = ETagAct::Beam_HangingMoveLeft;

	if (std::shared_ptr<ActionClip> pAct = pChar->GetActions((uint8_t)eAct))
		pChar->PlayActionClip(pAct, 0.1f);
};
