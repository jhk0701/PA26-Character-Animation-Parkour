#pragma once
#include "Scene/Scene.h"

class Character;

namespace MiniEngine { class ActionClip; }

class TestScene : public MiniEngine::Scene
{
public:
	TestScene();
	virtual ~TestScene();

	virtual void Construct() override;
	virtual void BeginPlay() override;

private:
	std::shared_ptr<MiniEngine::Actor> BuildObstacle(const wchar_t* _path, const MiniEngine::Vector3& _pos, const MiniEngine::Vector3& _scale);
};