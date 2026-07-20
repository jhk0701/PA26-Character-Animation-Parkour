#pragma once
#include "Scene/Actor.h"
#include "Scene/PerceptionComponent.h"
#include "Content/PerceptionQueryTree.h"
#include "Content/CharacterStateMachine.h"

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

	void Jump();
	void InputCamRotate();
	void InputMovement(float _dt);
	void CheckCharacterState();
	void ProcessPerceptionResult();

	void InputJump();

	// getter setter
	void SetInputDir(const Vector2& _dir) { m_inputDir = _dir; }
	void SetState(EState _state) { m_state = _state; }
	void SetAnimBaseTrackInputAxis(const Vector2& _input);

	float GetCapsuleRadius() const { return m_capsuleRadius; }
	float GetCapsuleHalfHeight() const { return m_capsuleHeight * 0.5f; }
	
	float GetMoveSpeed() const { return m_moveSpeed; }
	Vector2 GetInputDir() const { return m_inputDir; }
	float GetInputLerpWeight() const { return m_lerpWeight; }
	Vector2& InputLerp() { return m_lerpInputDir; }

	EState GetCharState() const { return m_state; }
	bool IsActionClipPlaying() const;

	std::weak_ptr<SkeletalMeshComponent> GetSkin() const { return m_skinMeshComp; }
	std::weak_ptr<CharacterControllerComponent> GetController() const { return m_charCont; }
	std::shared_ptr<ActionClip> GetActions(uint8_t _act) { return m_mapActions[_act]; }

	void SetEnableCollisionObstacle(bool _bEnable);
	void AddMovementInput(const Vector3& _moveDelta);

	void SetHangingState(bool _bIsOn);
	
	Actor* GetCurObstacle() const { return m_pCurObstacle; }
	float GetCurObstacleDistance() const { return m_curObstacleDistance; }
	float GetCurObstacleLedge() const { return m_curObstacleLedge + m_ledgeOffset; }
	Vector3 GetCurObstacleHitPos() const;

	void ResetCamRot() {/* m_camRotate.x = 180.0f; */m_camRotate.y = 180.0f; }

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

