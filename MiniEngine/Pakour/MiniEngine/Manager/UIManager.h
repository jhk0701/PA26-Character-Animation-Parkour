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

		// ImGui 가 마우스/키보드 입력을 캡처 중인지(게임 입력 게이트용).
		// 컨텍스트 미생성/미초기화면 false.
		bool WantCaptureMouse() const;
		bool WantCaptureKeyboard() const;

		template<typename UIType>
		std::weak_ptr<UIType> CreateUI();

	private:
		bool m_initialized = false;
		std::vector<std::shared_ptr<UIBase>> m_uiInsts;
	};

	// WndProc 포워딩(main.cpp 에서 호출). ImGui 가 메시지를 소비하면 non-zero 반환.
	LRESULT WndProcHandler(HWND _hWnd, UINT _msg, WPARAM _wParam, LPARAM _lParam);

	template<typename UIType>
	inline std::weak_ptr<UIType> UIManager::CreateUI()
	{
		std::shared_ptr<UIType> pInst = std::make_shared<UIType>();
		m_uiInsts.push_back(pInst);

		pInst->Construct();

		return pInst;
	}
}