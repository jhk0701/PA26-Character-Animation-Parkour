#pragma once
#include "Scene/Actor.h"
#include "Content/PerceptionQueryTree.h"
#include "Scene/PerceptionComponent.h"
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
	void SetMoveSpeed(float _newSpeed) { m_moveSpeed = _newSpeed; }
	void SetInputDir(const Vector2& _dir) { m_inputDir = _dir; }
	void SetState(EState _state) { m_state = _state; }

	Vector2 GetInputDir() const { return m_inputDir; }
	std::weak_ptr<SkeletalMeshComponent> GetSkin() const { return m_skinMeshComp; }
	std::weak_ptr<Animator> GetAnim() const;
	std::weak_ptr<CharacterControllerComponent> GetController() const { return m_charCont; }
	std::shared_ptr<ActionClip> GetActions(uint8_t _act) { return m_mapActions[_act]; }
	EState GetCharState() const { return m_state; }
	
	float GetCapsuleRadius() const { return m_capsuleRadius; }
	float GetCapsuleHalfHeight() const { return m_capsuleHeight * 0.5f; }

	void SetEnableCollisionObstacle(bool _bEnable);
	void SetHangingState(bool _bIsOn);
	
	Actor* GetCurObstacle() const { return m_pCurObstacle; }
	float GetCurObstacleDistance() const { return m_curObstacleDistance; }
	Vector3 GetCurObstacleHitPos() const { return m_curObstacleHitPos; }

	void ResetCamRot() {/* m_camRotate.x = 180.0f; */m_camRotate.y = 180.0f; }

private:
	void InitCollisionLayer();
	void InitInput();

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
	
	// 정리 필요
	Actor* m_pCurObstacle{ nullptr };
	float m_curObstacleDistance{ 0.0f };
	Vector3 m_curObstacleHitPos{ 0.0f };
};

