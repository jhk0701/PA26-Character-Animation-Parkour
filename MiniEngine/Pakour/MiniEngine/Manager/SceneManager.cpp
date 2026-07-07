#include "pch.h"
#include "Manager/SceneManager.h"
#include "Scene/World.h"

// TODO : 콘텐츠 코드. 엔진 코드에서 지울 것
#include "Content/TestScene.h"

namespace MiniEngine
{
	SceneManager::SceneManager() {};
	SceneManager::~SceneManager() {};

	void SceneManager::Init() 
	{
		// TODO : Config 읽어오기
		m_pCurScene = std::make_shared<TestScene>();
		
		if (m_pCurScene)
			m_pCurScene->Construct();
	}

	void SceneManager::BeginPlay()
	{
		if (m_pCurScene)
			m_pCurScene->BeginPlay();
	}

	void SceneManager::Update(float _dt)
	{
		if (!m_pCurScene)
			return;

		m_pCurScene->Tick(_dt);
	}

	void SceneManager::Render()
	{
		if (!m_pCurScene)
			return;

		m_pCurScene->Render();
	}
	void SceneManager::EndPlay()
	{
	}
};