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
	void InitAnimation(std::shared_ptr<SkeletalMeshComponent>& _skinComp);

	virtual void BeginPlay() override;
	virtual void Tick(float _dt) override;

	void ProcessInput(float _dt);
	void ProcessPerceptionResult();
	void CheckCharacterState();

	void SetMoveSpeed(float _newSpeed) { m_moveSpeed = _newSpeed; }
	void SetInputDir(const Vector2& _dir);
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

private:
	void InitInput();

	Vector2 m_inputDir;
	Vector2 m_lerpInputDir;
	float m_lerpWeight{ 2.0f };
	float m_moveSpeed{ 6.0f };
	float m_jumpSpeed{ 6.0f };

	float m_camRotateSpeed{ 0.3f };
	Vector2 m_camRotate{ 0.0f, 0.0f }; // yaw, pitch
	float m_camMaxPitchDeg{ 85.0f };

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

