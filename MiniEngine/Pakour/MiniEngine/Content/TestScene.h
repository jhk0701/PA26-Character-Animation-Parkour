#pragma once
#include "Scene/World.h"

class TestScene : public MiniEngine::World
{
public:
	TestScene();
	virtual ~TestScene();

	virtual void Construct() override;

private:
	std::shared_ptr<MiniEngine::Actor> BuildObstacle(const wchar_t* _path);
	
};