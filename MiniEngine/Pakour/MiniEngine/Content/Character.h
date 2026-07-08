#pragma once
#include "Scene/Actor.h"

namespace MiniEngine 
{
	class RigidBodyComponent;
	class SkeletalMeshComponent;
	class Animator;
	class BlendClip;
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

	void SetMoveSpeed(float _newSpeed) { m_moveSpeed = _newSpeed; }
	void SetInputDir(const Vector2& _dir);
	Vector2 GetInputDir() const { return m_inputDir; }

	std::weak_ptr<SkeletalMeshComponent> GetSkin() const { return m_skinMeshComp; }
	std::weak_ptr<Animator> GetAnim() const;

	void SetTempLoco(std::weak_ptr<BlendClip> _blendClip) { m_tempLoco = _blendClip; }
	std::weak_ptr<BlendClip> GetTempLoco() const { return m_tempLoco; }


private:
	// 이동 기능 -> 시간 남으면 CharacterMovementComponent로 리팩터링
	Vector2 m_inputDir;
	float m_moveSpeed{ 1.0f };

	std::weak_ptr<RigidBodyComponent> m_rigidBodyComp;
	std::weak_ptr<SkeletalMeshComponent> m_skinMeshComp;
	std::weak_ptr<BlendClip> m_tempLoco;
};

