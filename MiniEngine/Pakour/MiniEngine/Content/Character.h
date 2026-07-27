#pragma once
#include "Scene/Pawn.h"
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
	class IObstacle;
}

using namespace MiniEngine;

class CharacterStateMachine;
class Character : public Pawn
{
public:
	enum class EState : uint8_t
	{
		Landing,
		InAir,
		Hanging,
		BeamStand,
		BeamHanging,
		ProtrudeHanging,

		End
	};

	struct PerceptedObstacleInfo
	{
		uint8_t m_actTag;
		IObstacle* m_pObstacle{ nullptr };
		float m_obstacleDistance{ 0.0f };
		float m_obstacleLedge{ 0.0f };
		Vector3 m_obstacleHitPos{ 0.0f };
		Vector3 m_obstacleHitNrm{ 0.0f };

		bool IsValid() const { return m_pObstacle != nullptr; }
	};

	Character();
	virtual ~Character();

	void Construct(const Vector3& _initPosition);
	void PostConstruct();
	void InitAnimation(std::shared_ptr<SkeletalMeshComponent>& _skinComp);

	virtual void BeginPlay() override;

	void TryPerception();
	void ProcessPerceptionResult(const TravelResult& _result);

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
	float GetInputLerpWeight() const { return std::clamp(m_lerpWeight, 0.0f, 1.0f); }
	Vector2& InputLerp() { return m_lerpInputDir; }

	// 애니메이션
	void SetAnimBaseTrackInputAxis(const Vector2& _input);
	void TranstionBaseTrack(uint8_t _state, float _transitionTime = 0.25f);
	bool IsActionClipPlaying() const;
	std::shared_ptr<ActionClip> GetActions(uint8_t _act) { return m_mapActions[_act]; }
	void PlayActionClip(std::shared_ptr<ActionClip> _clip, float _transitionTime = 0.25f, uint8_t _priority = 0U);

	// 콜라이더 및 물리
	void AddMovementInput(const Vector3& _moveDelta);
	void SetPosition(const Vector3& _newPos);
	void ClearMovement();
	float GetCapsuleRadius() const { return m_capsuleRadius; }
	float GetCapsuleHalfHeight() const { return m_capsuleHeight * 0.5f; }
	float GetCharacterHalfHeight() const { return (m_capsuleHeight + m_capsuleRadius) * 0.5f; }

	void SetEnableCollisionObstacle(bool _bEnable);
	bool IsFalling() const;
	bool IsGrounded() const;
	void SetUseGravity(bool _bUse);

	// 상태머신
	void TransitionStateMachine(uint8_t _state);
	
	// 지형 인식
	PerceptedObstacleInfo& GetCurObstacleInfo() { return m_curObstacleInfo; };

private:
	void InitCollisionLayer();

	std::weak_ptr<Animator> GetAnim() const;

	// tag - action 맵 : 모션 테이블, 판단한 tag를 기반으로 O(1)로 찾을 것
	std::unordered_map<uint8_t, std::shared_ptr<ActionClip>> m_mapActions;

	Vector2 m_inputDir;
	Vector2 m_lerpInputDir;
	float m_lerpWeight{ 0.25f };
	float m_moveSpeed{ 6.0f };
	float m_jumpSpeed{ 6.0f };

	float m_capsuleRadius{ 0.25f };
	float m_capsuleHeight{ 1.6f };
	float m_capsuleContactOffset{ 0.05f };
	float m_stepOffset{ 0.2f };

	EState m_state{ EState::Landing };
	
	std::weak_ptr<SkeletalMeshComponent> m_skinMeshComp;
	std::weak_ptr<CharacterControllerComponent> m_charCont;

	PerceptionQueryTree m_perceptQueryTree;
	std::weak_ptr<PerceptionComponent> m_perception;
	std::weak_ptr<CharacterStateMachine> m_charFSM;
	
	PerceptedObstacleInfo m_curObstacleInfo;
};

