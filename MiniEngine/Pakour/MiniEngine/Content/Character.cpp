#include "pch.h"
#include "Character.h"

Character::Character()
{
}

Character::~Character()
{
}

void Character::BeginPlay()
{
	Actor::BeginPlay();
}

void Character::Tick(float _dt)
{
	Actor::Tick(_dt);

	float deltaSpeed = _dt * m_moveSpeed;
	Vector3 moveVec = Vector3(deltaSpeed * m_inputDir.y, 0.0f, deltaSpeed * m_inputDir.x);
	std::shared_ptr<SceneComponent> root = GetRoot();
	root->localTransform.position += moveVec;

}
