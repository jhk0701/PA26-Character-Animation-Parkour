#pragma once
#include "Scene/Actor.h"
#include "Scene/PerceptionComponent.h"
#include "Content/PerceptionQueryTree.h"

#include <unordered_map>

namespace MiniEngine 
{
	class RigidBodyComponent;
	class SkeletalMeshComponent;
	class CharacterControllerComponent;
	class Animator;
	class BlendClip;
	class ActionClip;
}

using namespace MiniEngine;

class CharacterStateMachine;
class Character : public Actor
{
public:
	enum class EState : uint8_t
	{
		Landing,
		InAir,
		Hanging,

		END
	};

	Character();
	virtual ~Character();

	void Construct();
	void PostConstruct();
	void InitAnimation(std::shared_ptr<SkeletalMeshComponent>& _skinComp);

	virtual void BeginPlay() override;
	virtual void Tick(float _dt) override;

	void ProcessPerceptionResult();

	void Jump();
	void InputJump();

	// Component Getter
	std::weak_ptr<SkeletalMeshComponent> GetSkin() const { return m_skinMeshComp; }
	std::weak_ptr<CharacterControllerComponent> GetController() const { return m_charCont; }

	// getter setter
	void SetState(EState _state) { m_state = _state; }
	EState GetState() const { return m_state; }
	
	// input 조작
	void SetInputDir(const Vector2& _dir) { m_inputDir = _dir; }
	Vector2 GetInputDir() const { return m_inputDir; }
	float GetMoveSpeed() const { return m_moveSpeed; }
	float GetInputLerpWeight() const { return m_lerpWeight; }
	Vector2& InputLerp() { return m_lerpInputDir; }

	// 카메라
	float GetCamRotateSpeed() const { return m_camRotateSpeed; }
	float GetCamPitchMaxDeg() const { return m_camPitchMaxDeg; }
	Vector2& CamRotate() { return m_camRotate; }
	std::weak_ptr<SceneComponent> GetCamHolder() const { return m_cameraHolder; }
	void ResetCamRot() {/* m_camRotate.x = 180.0f; */ m_camRotate.y = 180.0f; }

	// 애니메이션
	void SetAnimBaseTrackInputAxis(const Vector2& _input);
	void TranstionBaseTrack(uint8_t _state, float _transitionTime = 0.25f);
	bool IsActionClipPlaying() const;
	std::shared_ptr<ActionClip> GetActions(uint8_t _act) { return m_mapActions[_act]; }
	void PlayActionClip(std::shared_ptr<ActionClip> _clip, float _transitionTime = 0.25f);

	// 콜라이더 및 물리
	float GetCapsuleRadius() const { return m_capsuleRadius; }
	float GetCapsuleHalfHeight() const { return m_capsuleHeight * 0.5f; }
	void SetEnableCollisionObstacle(bool _bEnable);
	void AddMovementInput(const Vector3& _moveDelta);
	bool IsFalling() const;
	bool IsGrounded() const;
	void SetUseGravity(bool _bUse);

	// 상태머신
	void TransitionStateMachine(uint8_t _state);
	
	// 지형 인식
	Actor* GetCurObstacle() const { return m_pCurObstacle; }
	float GetCurObstacleDistance() const { return m_curObstacleDistance; }
	float GetCurObstacleLedge() const { return m_curObstacleLedge + m_ledgeOffset; }
	Vector3 GetCurObstacleHitPos() const;

private:
	void InitCollisionLayer();
	void InitInput();
	std::weak_ptr<Animator> GetAnim() const;

	// tag - action 맵 : 모션 테이블, 판단한 tag를 기반으로 O(1)로 찾을 것
	std::unordered_map<uint8_t, std::shared_ptr<ActionClip>> m_mapActions;

	Vector2 m_inputDir;
	Vector2 m_lerpInputDir;
	float m_lerpWeight{ 5.0f };
	float m_moveSpeed{ 6.0f };
	float m_jumpSpeed{ 6.0f };

	float m_camRotateSpeed{ 20.0f };
	float m_camPitchMaxDeg{ 85.0f };
	Vector2 m_camRotate{ 0.0f, 0.0f }; // yaw, pitch

	float m_capsuleRadius{ 0.25f };
	float m_capsuleHeight{ 1.5f };

	EState m_state{ EState::Landing };
	
	std::weak_ptr<SceneComponent> m_cameraHolder;
	std::weak_ptr<SkeletalMeshComponent> m_skinMeshComp;
	std::weak_ptr<CharacterControllerComponent> m_charCont;

	PerceptionQueryTree m_perceptQueryTree;
	std::weak_ptr<PerceptionComponent> m_perception;
	std::weak_ptr<CharacterStateMachine> m_charFSM;
	
	// 정리 필요
	Actor* m_pCurObstacle{ nullptr };
	float m_curObstacleDistance{ 0.0f };
	float m_curObstacleLedge{ 0.0f };
	float m_ledgeOffset{ 0.5f };
	Vector3 m_curObstacleHitPos{ 0.0f };
};

