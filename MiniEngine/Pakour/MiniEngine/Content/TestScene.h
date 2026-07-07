#pragma once
#include "Scene/Scene.h"

class TestScene : public MiniEngine::Scene
{
public:
	TestScene();
	virtual ~TestScene();

	virtual void Construct() override;

private:
	std::shared_ptr<MiniEngine::Actor> BuildObstacle(const wchar_t* _path);
	
};