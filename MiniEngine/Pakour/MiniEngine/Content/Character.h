#pragma once
#include "Scene/Pawn.h"
#include "Content/Data/PerceptionQueryTree.h"
#include "Content/Data/CharacterPerceptionConfig.h"
#include "Scene/PerceptionComponent.h"
#include "Animation/IK/LimbIKComponent.h"

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

	static bool TryParseState(const std::string& _name, uint8_t& _outState);
	static const char* GetStateName(uint8_t _state);

	struct PerceptedObstacleInfo
	{
		// uint8_t m_actTag;
		IObstacle* m_pObstacle{ nullptr };
		float m_obstacleDistance{ 0.0f };
		float m_obstacleLedge{ 0.0f };
		float m_obstacleDepth{ 0.0f };
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
	virtual void OnBeforeSortComponent() override;
	void Tick(float _dt) override;
	// void LateTick(float _dt) override;

	void LoadData();

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
	
	// IK
	void ReserveIKDetectGround();
	LimbIKComponent::TaskResult IKDetectGround(uint8_t _ik);
	void ReserveIKDetectWall();
	LimbIKComponent::TaskResult IKDetectWall(uint8_t _ik);
	void ReserveIKDetectBeamHanging();
	LimbIKComponent::TaskResult IKDetectBeamHanging(uint8_t _ik);
	void ClearIKReserve();

	// 노티파이를 통해서 호출될 것
	void IKDetectObstacle(uint8_t _ik, const Vector3& _posOffset);
	void IKSetFixedPoint(uint8_t _ik, const Vector3& _posOffset);

	void SetIKAlpha(uint8_t _ik, float _alpha);

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

	// 지형 인식
	PerceptedObstacleInfo& GetCurObstacleInfo() { return m_curObstacleInfo; };
	const PerceptionConfig& GetPerceptionConfig() const;

	// 상태머신
	void TransitionStateMachine(uint8_t _state);

private:
	void InitCollisionLayer();

	std::weak_ptr<Animator> GetAnim() const;

	// tag - action 맵 : 모션 테이블, 판단한 tag를 기반으로 O(1)로 찾을 것
	std::unordered_map<uint8_t, std::shared_ptr<ActionClip>> m_mapActions;

	Vector2 m_inputDir;
	Vector2 m_lerpInputDir;
	float m_lerpWeight{ 0.1f };
	float m_moveSpeed{ 6.0f };
	float m_jumpSpeed{ 6.0f };

	float m_capsuleRadius{ 0.25f };
	float m_capsuleHeight{ 1.6f };
	float m_capsuleContactOffset{ 0.05f };
	float m_stepOffset{ 0.2f };

	EState m_state{ EState::Landing };
	
	std::weak_ptr<SkeletalMeshComponent> m_skinMeshComp;
	std::weak_ptr<LimbIKComponent> m_limbIKComp;
	std::weak_ptr<CharacterControllerComponent> m_charCont;

	std::weak_ptr<CharacterPerceptionConfig> m_pPerceptionConfig;
	PerceptionQueryTree m_perceptQueryTree;

	PerceptedObstacleInfo m_curObstacleInfo;
	std::weak_ptr<PerceptionComponent> m_perception;
	std::weak_ptr<CharacterStateMachine> m_charFSM;

	// IK 튜닝 멤버 변수
	float m_ikRayDistance = 0.5f; // 발의 아래, 벽면 등 레이로 체크할 거리

	// 발/골반이 애니 포즈 대비 벗어날 수 있는 한계
	float m_maxSlopeDeg = 170.0f; // 급경사 등 발이 뒤집히지 않게하는 최대 각도
	float m_thresholdWallDig = 0.1f;
};

