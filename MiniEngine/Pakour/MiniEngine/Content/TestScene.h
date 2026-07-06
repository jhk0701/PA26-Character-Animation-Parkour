#pragma once
#include "Scene/World.h"

class TestScene : public MiniEngine::World
{
public:
	TestScene();
	~TestScene();

	void Construct() override;

private:
	std::shared_ptr<MiniEngine::Actor> BuildObstacle(const std::wstring& _path);

};