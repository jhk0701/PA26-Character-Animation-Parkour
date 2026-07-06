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
}
