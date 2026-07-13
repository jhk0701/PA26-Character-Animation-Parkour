#pragma once
#include "Scene/Actor.h"

#include <unordered_map>

namespace MiniEngine 
{
	class RigidBodyComponent;
	class SkeletalMeshComponent;
	class CharacterControllerComponent;
	class PerceptionComponent;
	class Animator;
	class BlendClip;
	class ActionClip;
}

using namespace MiniEngine;

class Character : public MiniEngine::Actor
{
public:
	enum class EState 
	{
		Landing,
		Hanging,
		InAir,
	};

	Character();
	virtual ~Character();

	void Construct();
	virtual void BeginPlay() override;
	virtual void Tick(float _dt) override;

	void ProcessInput(float _dt);

	void SetMoveSpeed(float _newSpeed) { m_moveSpeed = _newSpeed; }
	void SetInputDir(const Vector2& _dir);
	void SetCamRotDir(const Vector2& _dir);

	Vector2 GetInputDir() const { return m_inputDir; }
	Vector2 GetCamRotDir() const { return m_camRotDir; }
	std::weak_ptr<SkeletalMeshComponent> GetSkin() const { return m_skinMeshComp; }
	std::weak_ptr<Animator> GetAnim() const;
	std::shared_ptr<ActionClip> GetActions(uint8_t _act) { return m_mapActions[_act]; }
	
	EState GetCharState() const { return m_state; }
	float GetCapsuleRadius() const { return m_capsuleRadius; }
	float GetCapsuleHeight() const { return m_capsuleHeight; }

private:
	void InitInput();

	// 이동 기능 -> 시간 남으면 CharacterMovementComponent로 리팩터링
	Vector2 m_inputDir;
	Vector2 m_lerpInputDir;
	float m_lerpWeight{ 2.0f };
	float m_moveSpeed{ 6.0f };
	float m_jumpSpeed{ 6.0f };

	Vector2 m_camRotDir;
	float m_camRotateSpeed{ 1.0f };

	float m_capsuleRadius{ 0.25f };
	float m_capsuleHeight{ 1.5f };

	EState m_state{ EState::Landing };
	
	std::weak_ptr<SceneComponent> m_cameraHolder;
	std::weak_ptr<SkeletalMeshComponent> m_skinMeshComp;
	std::weak_ptr<CharacterControllerComponent> m_charCont;
	std::weak_ptr<PerceptionComponent> m_perception;

	// tag - action 맵
	std::unordered_map<uint8_t, std::shared_ptr<ActionClip>> m_mapActions;
};

