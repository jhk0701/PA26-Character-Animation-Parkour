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

	void InitDefaultInput(); // 기본 공통사항 인풋 바인딩

private:
	std::shared_ptr<MiniEngine::Actor> BuildObstacle(const wchar_t* _path);

	std::shared_ptr<Character> m_TmpChar;
	std::shared_ptr<MiniEngine::ActionClip> m_ActionClip;
};