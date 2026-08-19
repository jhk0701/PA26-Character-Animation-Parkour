#include "pch.h"
#include "Manager/SceneManager.h"
#include "Scene/Scene.h"

#if defined(WITH_EDITOR)
#include "Editor/EditorScene.h"
#else
// TODO : 콘텐츠 코드. 엔진 코드에서 지울 것
#include "Content/TestScene.h"
#endif

namespace MiniEngine
{
	SceneManager::SceneManager() {};
	SceneManager::~SceneManager() {};

	void SceneManager::Init(ID3D11Device* _device, ID3D11DeviceContext* _context)
	{
		// TODO : Config 읽어오기
		// 에디터 구성은 파쿠르 콘텐츠 대신 빈 에디터 씬을 띄운다.
#if defined(WITH_EDITOR)
		m_pCurScene = std::make_shared<EditorScene>();
#else
		m_pCurScene = std::make_shared<TestScene>();
#endif
		
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

	void SceneManager::LateUpdate(float _dt)
	{
		if (!m_pCurScene)
			return;

		m_pCurScene->LateTick(_dt);
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