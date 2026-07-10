#pragma once
#include "Scene/Actor.h"

namespace MiniEngine 
{
	class RigidBodyComponent;
	class SkeletalMeshComponent;
	class Animator;
	class BlendClip;
	class ActionClip;
}

using namespace MiniEngine;

class Character : public MiniEngine::Actor
{
public:
	Character();
	virtual ~Character();

	void Construct();
	virtual void BeginPlay() override;
	virtual void Tick(float _dt) override;

	void ProcessInput(float _dt);
	void ProcessRootMotion();

	void SetMoveSpeed(float _newSpeed) { m_moveSpeed = _newSpeed; }
	void SetInputDir(const Vector2& _dir);
	void SetCamRotDir(const Vector2& _dir);

	Vector2 GetInputDir() const { return m_inputDir; }
	Vector2 GetCamRotDir() const { return m_camRotDir; }
	std::weak_ptr<SkeletalMeshComponent> GetSkin() const { return m_skinMeshComp; }
	std::weak_ptr<Animator> GetAnim() const;

private:
	void InitInput();
	bool RaycastObstacle();

	// 이동 기능 -> 시간 남으면 CharacterMovementComponent로 리팩터링
	Vector2 m_inputDir;
	Vector2 m_lerpInputDir;
	float m_lerpWeight{ 2.0f };
	float m_moveSpeed{ 4.0f };

	Vector2 m_camRotDir;
	float m_camRotateSpeed{ 1.0f };
	
	std::weak_ptr<SceneComponent> m_cameraHolder;
	std::weak_ptr<RigidBodyComponent> m_rigidBodyComp;

	std::weak_ptr<SkeletalMeshComponent> m_skinMeshComp;
	std::shared_ptr<BlendClip> m_tempLoco;
	std::shared_ptr<ActionClip> m_tempJump;
	std::shared_ptr<ActionClip> m_tempActionClip;
};

