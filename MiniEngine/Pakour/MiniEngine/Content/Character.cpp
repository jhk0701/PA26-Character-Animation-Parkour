#include "pch.h"
#include "Character.h"
#include "Core/Math.h"

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

	if (m_inputDir.LengthSquared() > 0) 
	{
		// 임시 이동 코드
		const float deltaSpeed = _dt * m_moveSpeed;
		std::shared_ptr<SceneComponent> root = GetRoot();

		// 캐릭터 정면 기준 이동
		const Vector3& fwd = root->localTransform.GetMatrix().Forward();
		const Vector3& rht = root->localTransform.GetMatrix().Right();

		root->localTransform.position +=
			deltaSpeed * m_inputDir.y * fwd +
			deltaSpeed * m_inputDir.x * rht;
	}
}
