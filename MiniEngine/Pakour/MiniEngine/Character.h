#pragma once
#include "Scene/Actor.h"

class Character : public MiniEngine::Actor
{
public:
	Character();
	virtual ~Character();

	virtual void BeginPlay() override;
	virtual void Tick(float _dt) override;
};

