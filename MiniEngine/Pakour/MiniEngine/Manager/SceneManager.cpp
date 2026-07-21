#include "pch.h"
#include "Manager/SceneManager.h"
#include "Scene/Scene.h"

// TODO : 콘텐츠 코드. 엔진 코드에서 지울 것
#include "Content/TestScene.h"

namespace MiniEngine
{
	SceneManager::SceneManager() {};
	SceneManager::~SceneManager() {};

	void SceneManager::Init(ID3D11Device* _device, ID3D11DeviceContext* _context)
	{
		// TODO : Config 읽어오기
		m_pCurScene = std::make_shared<TestScene>();
		
		if (m_pCurScene)
			m_pCurScene->Construct(_device, _context);
	}

	void SceneManager::BeginPlay()
	{
		if (!m_pCurScene)
			return;

		m_pCurScene->BeginPlay();
	}

	void SceneManager::FixedUpdate(float _dt)
	{
		if (!m_pCurScene)
			return;
		
		m_pCurScene->FixedTick(_dt);
	}

	void SceneManager::Update(float _dt)
	{
		if (!m_pCurScene)
			return;

		m_pCurScene->Tick(_dt);
	}

	void SceneManager::Render(Graphics::RenderContext& _context)
	{
		if (!m_pCurScene)
			return;

		m_pCurScene->Render(_context);
	}

	void SceneManager::EndPlay()
	{
		if (!m_pCurScene)
			return;

		m_pCurScene->EndPlay();
	}
};