#pragma once
#include <windows.h>
#include <string>   
#include <vector>
#include "Core/Math.h"

struct ID3D11Device;
struct ID3D11DeviceContext;

namespace MiniEngine 
{
	class UIBase;
	class Scene;
	class UIManager
	{
		SINGLETON(UIManager)

	public:
		bool Init(HWND _hWnd, ID3D11Device* _device, ID3D11DeviceContext* _context);
		void Shutdown();
		void BuildUI(Scene& _world, const Matrix& _view, const Matrix& _proj);
		void Render();

		template<typename UIType>
		std::weak_ptr<UIType> CreateUI();

	private:
		bool m_initialized = false;
		std::vector<std::shared_ptr<UIBase>> m_uiInsts;
	};

	template<typename UIType>
	inline std::weak_ptr<UIType> UIManager::CreateUI()
	{
		std::shared_ptr<UIType> pInst = std::make_shared<UIType>();
		m_uiInsts.push_back(pInst);

		pInst->Construct();

		return pInst;
	}
}