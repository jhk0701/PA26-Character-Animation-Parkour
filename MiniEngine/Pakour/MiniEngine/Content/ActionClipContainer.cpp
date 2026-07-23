#include "pch.h"
#include "Content/ActionClipContainer.h"
#include "Asset/SkinnedMesh.h"
#include "Animation/Animator.h"
#include "Animation/BlendClip.h"
#include "Animation/ActionClip.h"
#include "Animation/AnimNotify.h"

#include "Content/ContentConfig.h"
#include "Content/Character.h"

// 노티파이
#include "Content/JumpTiming.h"
#include "Content/EnableCollisionObstacle.h"
#include "Content/TransitionState.h"
#include "Content/CorrectRootMotion.h"
#include "Content/UseGravity.h"

using namespace MiniEngine;
using namespace Content::Config;

void ActionClipContainer::LoadActionClips(ActionClipLoadParam& _param)
{
	std::shared_ptr<Animator> pAnim = _param.pAnim;
	std::shared_ptr<SkinnedMesh> skinnedMesh = _param.pSources;
	std::unordered_map<uint8_t, std::shared_ptr<MiniEngine::ActionClip>>& mapActions = *_param.pMaps;

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
	{
		// Beam Stand
		std::shared_ptr<BlendClip> pBlend = std::make_shared<BlendClip>(1);
		// 모션 입력 
		pBlend->AddAnimClip({ 0, 0 }, skinnedMesh->GetClipPtr(38)); // A_Balance_Idle
		pBlend->AddAnimClip({ 0, 1 }, skinnedMesh->GetClipPtr(39)); // A_Balance_Run

		pAnim->AddBaseLocomotion(pBlend);
	}
	{
		// Beam Hanging
		std::shared_ptr<BlendClip> pBlend = std::make_shared<BlendClip>(1);
		// 모션 입력 
		pBlend->AddAnimClip({ 0, 0 }, skinnedMesh->GetClipPtr(32)); // A_Hanging_Idle

		pAnim->AddBaseLocomotion(pBlend);
	}

	// ActionClip 구성
	{
		// Jump
		std::shared_ptr<ActionClip> pActionClip = std::make_shared<ActionClip>();
		pActionClip->AddClip(skinnedMesh->GetClipPtr(9));
		pActionClip->SetApplyRootBone(false); // jump는 루트모션 적용하지 않음

		std::shared_ptr<JumpTiming> pJumpNotify = std::make_shared<JumpTiming>();
		pJumpNotify->SetTime(0.5f);
		pActionClip->AddNotify(pJumpNotify);

		mapActions[(uint8_t)Content::Config::ETagAct::Jump] = pActionClip;
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
		pCorrectRM->SetTime(0.0f, 0.2f);
		pCorrectRM->SetProperDistance(1.0f); // 적정거리 1.5 ~ 1.0
		pCorrectRM->SetLerpWeight(0.5f);
		pCorrectRM->SetDeltaIntensity(2.0f);
		pActionClip->AddNotify(pCorrectRM);

		mapActions[(uint8_t)ETagAct::VaultLow] = pActionClip;
		mapActions[(uint8_t)ETagAct::VaultMid] = pActionClip;
	}
	{
		// Vault High
		std::shared_ptr<ActionClip> pActionClip = std::make_shared<ActionClip>();
		pActionClip->AddClip(skinnedMesh->GetClipPtr(13));

		std::shared_ptr<EnableCollisionObstacle> pIgnoreObstacle = std::make_shared<EnableCollisionObstacle>();
		pIgnoreObstacle->SetTime(0.2f);
		pIgnoreObstacle->SetEnable(false);
		pActionClip->AddNotify(pIgnoreObstacle);

		std::shared_ptr<EnableCollisionObstacle> pCollideObstacle = std::make_shared<EnableCollisionObstacle>();
		pCollideObstacle->SetTime(1.0f);
		pCollideObstacle->SetEnable(true);
		pActionClip->AddNotify(pCollideObstacle);

		std::shared_ptr<CorrectRootMotion> pCorrectRM = std::make_shared<CorrectRootMotion>();
		pCorrectRM->SetTime(0.0f, 0.9f);
		pCorrectRM->SetProperDistance(0.5f); // 적정거리 1.5 ~ 1.0
		pCorrectRM->SetLerpWeight(0.85f);
		pActionClip->AddNotify(pCorrectRM);

		mapActions[(uint8_t)ETagAct::VaultHigh] = pActionClip;
	}
	{
		// Mantle_A_TwoHand_L_On
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
		pCorrectRM->SetCorrectAxis(ECorrectAxis::YZ);
		pCorrectRM->SetTime(0.01f, 0.5f);
		pCorrectRM->SetProperDistance(1.0f);
		pActionClip->AddNotify(pCorrectRM);

		mapActions[(uint8_t)ETagAct::MantleLow] = pActionClip;
		mapActions[(uint8_t)ETagAct::MantleMid] = pActionClip;
	}
	{
		// Mantle_A_Wall_Monkey_On
		// Mantle High
		std::shared_ptr<ActionClip> pActionClip = std::make_shared<ActionClip>();
		pActionClip->AddClip(skinnedMesh->GetClipPtr(15));

		std::shared_ptr<EnableCollisionObstacle> pIgnoreObstacle = std::make_shared<EnableCollisionObstacle>();
		pIgnoreObstacle->SetTime(0.3f);
		pIgnoreObstacle->SetEnable(false);
		pActionClip->AddNotify(pIgnoreObstacle);

		std::shared_ptr<EnableCollisionObstacle> pCollideObstacle = std::make_shared<EnableCollisionObstacle>();
		pCollideObstacle->SetTime(0.9f);
		pCollideObstacle->SetEnable(true);
		pActionClip->AddNotify(pCollideObstacle);

		std::shared_ptr<CorrectRootMotion> pCorrectRM = std::make_shared<CorrectRootMotion>();
		pCorrectRM->SetCorrectAxis(ECorrectAxis::YZ);
		pCorrectRM->SetTime(0.0f, 0.9f);
		pCorrectRM->SetLerpWeight(0.85f);
		pCorrectRM->SetProperDistance(0.5f);
		pActionClip->AddNotify(pCorrectRM);

		mapActions[(uint8_t)ETagAct::MantleHigh] = pActionClip;
	}
	{
		// Falling To Landing
		// FallingToLand
		std::shared_ptr<ActionClip> pActionClip = std::make_shared<ActionClip>();
		pActionClip->AddClip(skinnedMesh->GetClipPtr(11));
		pActionClip->SetApplyRootBone(false);
		mapActions[(uint8_t)ETagAct::FallingToLand] = pActionClip;
	}
	{
		// Idle To Braced Hang
		// IdleToHang
		std::shared_ptr<ActionClip> pActionClip = std::make_shared<ActionClip>();
		pActionClip->AddClip(skinnedMesh->GetClipPtr(16)); // 벽 매달리기 (시작)

		std::shared_ptr<TransitionState> pSetHanging = std::make_shared<TransitionState>();
		pSetHanging->SetTime(0.1f);
		pSetHanging->SetState((uint8_t)Character::EState::Hanging);
		pActionClip->AddNotify(pSetHanging);

		std::shared_ptr<CorrectRootMotion> pCorrectRM = std::make_shared<CorrectRootMotion>();
		pCorrectRM->SetTime(0.0f, 1.0f);
		pCorrectRM->SetProperDistance(0.05f);
		pCorrectRM->SetLerpWeight(0.95f);
		pActionClip->AddNotify(pCorrectRM);

		mapActions[(uint8_t)ETagAct::Wall_IdleToHang] = pActionClip;
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
		pSetLanding->SetState((uint8_t)Character::EState::Landing);
		pActionClip->AddNotify(pSetLanding);

		mapActions[(uint8_t)ETagAct::Wall_HangToIdle] = pActionClip;
	}
	{
		// A_Ledge_ClimbUp_Monkey_Mantle
		std::shared_ptr<ActionClip> pActionClip = std::make_shared<ActionClip>();
		pActionClip->AddClip(skinnedMesh->GetClipPtr(28)); // 벽에서 올라감

		std::shared_ptr<TransitionState> pSetLanding = std::make_shared<TransitionState>();
		pSetLanding->SetTime(0.9f);
		pSetLanding->SetState((uint8_t)Character::EState::Landing);
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
		pCorrectRM->SetCorrectAxis(ECorrectAxis::YZ);
		pCorrectRM->SetTime(0.0f, 0.5f);
		pCorrectRM->SetLerpWeight(0.5f);
		pCorrectRM->SetProperDistance(0.85f);
		pActionClip->AddNotify(pCorrectRM);

		mapActions[(uint8_t)ETagAct::Wall_HangToMantle] = pActionClip;
	}
	{
		// Wall_Hanging 중 이동 모션
		std::shared_ptr<ActionClip> pHangMoveUp = std::make_shared<ActionClip>();
		pHangMoveUp->AddClip(skinnedMesh->GetClipPtr(19));
		mapActions[(uint8_t)ETagAct::Wall_HangingMoveUp] = pHangMoveUp;

		std::shared_ptr<ActionClip> pHangMoveDown = std::make_shared<ActionClip>();
		pHangMoveDown->AddClip(skinnedMesh->GetClipPtr(20));
		mapActions[(uint8_t)ETagAct::Wall_HangingMoveDown] = pHangMoveDown;

		std::shared_ptr<ActionClip> pHangMoveLeft = std::make_shared<ActionClip>();
		pHangMoveLeft->AddClip(skinnedMesh->GetClipPtr(21));
		mapActions[(uint8_t)ETagAct::Wall_HangingMoveLeft] = pHangMoveLeft;

		std::shared_ptr<ActionClip> pHangMoveRight = std::make_shared<ActionClip>();
		pHangMoveRight->AddClip(skinnedMesh->GetClipPtr(22));
		mapActions[(uint8_t)ETagAct::Wall_HangingMoveRight] = pHangMoveRight;
	}
	{
		// A_Ledge_Jump180_L
		// 벽에서 매달린 상태에서 점프
		std::shared_ptr<ActionClip> pActionClip = std::make_shared<ActionClip>();
		pActionClip->AddClip(skinnedMesh->GetClipPtr(23));

		// 벽에서 매달렸을 것. 상태 전환 -> Jump로 전환이므로 InAir
		std::shared_ptr<TransitionState> pSetInAir = std::make_shared<TransitionState>();
		pSetInAir->SetTime(0.3f);
		pSetInAir->SetState((uint8_t)Character::EState::InAir);
		pActionClip->AddNotify(pSetInAir);

		mapActions[(uint8_t)ETagAct::JumpFromWall] = pActionClip;
	}
	{
		// 회전에 대한 디테일한 보정 필요
		// 벽에서 코너 돌기 Inner Left
		std::shared_ptr<ActionClip> pActionClip = std::make_shared<ActionClip>();
		pActionClip->AddClip(skinnedMesh->GetClipPtr(24));

		mapActions[(uint8_t)ETagAct::Wall_InnerRotateLeft] = pActionClip;
	}
	{
		// 벽에서 코너 돌기 Inner Right
		std::shared_ptr<ActionClip> pActionClip = std::make_shared<ActionClip>();
		pActionClip->AddClip(skinnedMesh->GetClipPtr(25));

		mapActions[(uint8_t)ETagAct::Wall_InnerRotateRight] = pActionClip;
	}
	{
		// 벽에서 코너 돌기 Outer Left
		std::shared_ptr<ActionClip> pActionClip = std::make_shared<ActionClip>();
		pActionClip->AddClip(skinnedMesh->GetClipPtr(26));

		mapActions[(uint8_t)ETagAct::Wall_OuterRotateLeft] = pActionClip;
	}
	{
		// 벽에서 코너 돌기 Outer Right
		std::shared_ptr<ActionClip> pActionClip = std::make_shared<ActionClip>();
		pActionClip->AddClip(skinnedMesh->GetClipPtr(27));

		mapActions[(uint8_t)ETagAct::Wall_OuterRotateRight] = pActionClip;
	}
	{
		// A_Jump_One_L
		// Beam_IdleToStand
		std::shared_ptr<ActionClip> pActionClip = std::make_shared<ActionClip>();
		pActionClip->AddClip(skinnedMesh->GetClipPtr(30));
		pActionClip->SetApplyRootBone(false);

		std::shared_ptr<TransitionState> pTransition = std::make_shared<TransitionState>();
		pTransition->SetState((uint8_t)Character::EState::BeamStand);
		pTransition->SetTime(0.6f);
		pActionClip->AddNotify(pTransition);

		std::shared_ptr<BezierCorrectRootMotion> pCorrectRM = std::make_shared<BezierCorrectRootMotion>();
		pCorrectRM->SetTime(0.0f, 0.5f);
		pCorrectRM->SetBezierY(1.0f);
		pActionClip->AddNotify(pCorrectRM);

		mapActions[(uint8_t)ETagAct::Beam_IdleToStand] = pActionClip;
	}
	{
		// Rotate Left
		std::shared_ptr<ActionClip> pRotateLeft = std::make_shared<ActionClip>();
		pRotateLeft->AddClip(skinnedMesh->GetClipPtr(40));
		
		std::shared_ptr<RotateMotion> pCorrectRotateLeft = std::make_shared<RotateMotion>();
		pCorrectRotateLeft->SetTime(0.1, 0.7);
		pCorrectRotateLeft->SetRotateDeg({-90.0f, 0.0f, 0.0f});
		pRotateLeft->AddNotify(pCorrectRotateLeft);

		mapActions[(uint8_t)ETagAct::Beam_StandRotateLeft] = pRotateLeft;

		// Rotate Right
		std::shared_ptr<ActionClip> pRotateRight = std::make_shared<ActionClip>();
		pRotateRight->AddClip(skinnedMesh->GetClipPtr(40));

		std::shared_ptr<RotateMotion> pCorrectRotateRight = std::make_shared<RotateMotion>();
		pCorrectRotateRight->SetTime(0.1, 0.7);
		pCorrectRotateRight->SetRotateDeg({ 90.0f, 0.0f, 0.0f });
		pRotateRight->AddNotify(pCorrectRotateRight);

		mapActions[(uint8_t)ETagAct::Beam_StandRotateRight] = pRotateRight;
	}
	{
		// Beam_StandToIdle
		std::shared_ptr<ActionClip> pActionClip = std::make_shared<ActionClip>();
		pActionClip->AddClip(skinnedMesh->GetClipPtr(30));
		pActionClip->SetApplyRootBone(false);

		std::shared_ptr<TransitionState> pTransition = std::make_shared<TransitionState>();
		pTransition->SetState((uint8_t)Character::EState::Landing);
		pTransition->SetTime(0.6f);
		pActionClip->AddNotify(pTransition);

		std::shared_ptr<BezierCorrectRootMotion> pCorrectRM = std::make_shared<BezierCorrectRootMotion>();
		pCorrectRM->SetTime(0.0f, 0.5f);
		pCorrectRM->SetBezierY(1.0f);
		pActionClip->AddNotify(pCorrectRM);

		mapActions[(uint8_t)ETagAct::Beam_StandToIdle] = pActionClip;
	}
	{
		// Beam_IdleToHanging
		std::shared_ptr<ActionClip> pActionClip = std::make_shared<ActionClip>();
		pActionClip->AddClip(skinnedMesh->GetClipPtr(31));

		std::shared_ptr<TransitionState> pTransition = std::make_shared<TransitionState>();
		pTransition->SetState((uint8_t)Character::EState::BeamHanging);
		pTransition->SetTime(0.9f);
		pActionClip->AddNotify(pTransition);

		std::shared_ptr<EnableCollisionObstacle> pIgnoreCollision = std::make_shared<EnableCollisionObstacle>();
		pIgnoreCollision->SetEnable(false);
		pIgnoreCollision->SetTime(0.0f);
		pActionClip->AddNotify(pIgnoreCollision);

		std::shared_ptr<EnableCollisionObstacle> pEnableCollision = std::make_shared<EnableCollisionObstacle>();
		pEnableCollision->SetEnable(true);
		pEnableCollision->SetTime(0.9f);
		pActionClip->AddNotify(pEnableCollision);

		std::shared_ptr<BezierCorrectRootMotion> pCorrectRM = std::make_shared<BezierCorrectRootMotion>();
		pCorrectRM->SetTime(0.0f, 0.3f);
		pCorrectRM->SetEndOffset({0.0f, -2.2f, 0.0f});
		pActionClip->AddNotify(pCorrectRM);

		mapActions[(uint8_t)ETagAct::Beam_IdleToHang] = pActionClip;
	}
	{
		// Beam_HangingToIdle
		std::shared_ptr<ActionClip> pActionClip = std::make_shared<ActionClip>();
		pActionClip->AddClip(skinnedMesh->GetClipPtr(35));

		std::shared_ptr<TransitionState> pTransition = std::make_shared<TransitionState>();
		pTransition->SetState((uint8_t)Character::EState::Landing);
		pTransition->SetTime(0.9f);
		pActionClip->AddNotify(pTransition);

		std::shared_ptr<EnableCollisionObstacle> pIgnoreCollision = std::make_shared<EnableCollisionObstacle>();
		pIgnoreCollision->SetEnable(false);
		pIgnoreCollision->SetTime(0.0f);
		pActionClip->AddNotify(pIgnoreCollision);

		std::shared_ptr<EnableCollisionObstacle> pEnableCollision = std::make_shared<EnableCollisionObstacle>();
		pEnableCollision->SetEnable(true);
		pEnableCollision->SetTime(1.2f);
		pActionClip->AddNotify(pEnableCollision);

		std::shared_ptr<BezierCorrectRootMotion> pCorrectRM = std::make_shared<BezierCorrectRootMotion>();
		pCorrectRM->SetTime(0.8f, 1.3f);
		pActionClip->AddNotify(pCorrectRM);

		mapActions[(uint8_t)ETagAct::Beam_HangToIdle] = pActionClip;
	}

	// Beam Hanging 좌우 이동
	{
		std::shared_ptr<ActionClip> pHangMoveLeft = std::make_shared<ActionClip>();
		pHangMoveLeft->AddClip(skinnedMesh->GetClipPtr(36));

		std::shared_ptr<EnableCollisionObstacle> pIgnoreCollision = std::make_shared<EnableCollisionObstacle>();
		pIgnoreCollision->SetEnable(false);
		pIgnoreCollision->SetTime(0.0f);
		pHangMoveLeft->AddNotify(pIgnoreCollision);

		std::shared_ptr<EnableCollisionObstacle> pEnableCollision = std::make_shared<EnableCollisionObstacle>();
		pEnableCollision->SetEnable(true);
		pEnableCollision->SetTime(0.7f);
		pHangMoveLeft->AddNotify(pEnableCollision);

		mapActions[(uint8_t)ETagAct::Beam_HangingMoveLeft] = pHangMoveLeft;
	}
	{
		std::shared_ptr<ActionClip> pHangMoveRight = std::make_shared<ActionClip>();
		pHangMoveRight->AddClip(skinnedMesh->GetClipPtr(37));

		std::shared_ptr<EnableCollisionObstacle> pIgnoreCollision = std::make_shared<EnableCollisionObstacle>();
		pIgnoreCollision->SetEnable(false);
		pIgnoreCollision->SetTime(0.0f);
		pHangMoveRight->AddNotify(pIgnoreCollision);

		std::shared_ptr<EnableCollisionObstacle> pEnableCollision = std::make_shared<EnableCollisionObstacle>();
		pEnableCollision->SetEnable(true);
		pEnableCollision->SetTime(0.7f);
		pHangMoveRight->AddNotify(pEnableCollision);

		mapActions[(uint8_t)ETagAct::Beam_HangingMoveRight] = pHangMoveRight;
	}
		
	{
		// Beam Hanging 올라가기
		// Beam Hang -> Beam Stand
		std::shared_ptr<ActionClip> pActionClip = std::make_shared<ActionClip>();
		pActionClip->AddClip(skinnedMesh->GetClipPtr(33));

		std::shared_ptr<TransitionState> pTransition = std::make_shared<TransitionState>();
		pTransition->SetTime(1.6f);
		pTransition->SetState((uint8_t)Character::EState::BeamStand);
		pActionClip->AddNotify(pTransition);

		std::shared_ptr<EnableCollisionObstacle> pIgnoreCollision = std::make_shared<EnableCollisionObstacle>();
		pIgnoreCollision->SetEnable(false);
		pIgnoreCollision->SetTime(0.0f);
		pActionClip->AddNotify(pIgnoreCollision);

		std::shared_ptr<EnableCollisionObstacle> pEnableCollision = std::make_shared<EnableCollisionObstacle>();
		pEnableCollision->SetEnable(true);
		pEnableCollision->SetTime(1.6f);
		pActionClip->AddNotify(pEnableCollision);

		std::shared_ptr<BezierCorrectRootMotion> pCorrectRM = std::make_shared<BezierCorrectRootMotion>();
		pCorrectRM->SetTime(1.0f, 1.8f);
		pCorrectRM->SetEndOffset({0.0f, 0.0f, -0.25f});
		pActionClip->AddNotify(pCorrectRM);

		mapActions[(uint8_t)ETagAct::Beam_HangingMoveUp] = pActionClip;
	}
	{
		// Beam Hanging 내려가기
		// Beam Hang -> In Air
		std::shared_ptr<ActionClip> pActionClip = std::make_shared<ActionClip>();
		pActionClip->AddClip(skinnedMesh->GetClipPtr(34));

		std::shared_ptr<TransitionState> pTransition = std::make_shared<TransitionState>();
		pTransition->SetTime(0.2f);
		pTransition->SetState((uint8_t)Character::EState::InAir);
		pActionClip->AddNotify(pTransition);

		mapActions[(uint8_t)ETagAct::Beam_HangingMoveDown] = pActionClip;
	}
}
