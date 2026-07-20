#include "pch.h"
#include "Manager/UIManager.h"
#include "UI/UIBase.h"

#include <imgui.h>
#include <imgui_impl_win32.h>
#include <imgui_impl_dx11.h>
#include <ImGuizmo.h>

#if defined(WITH_EDITOR)

namespace MiniEngine 
{
	UIManager() {}
	~UIManager() {}

	bool UIManager::Initialize(HWND _hWnd, ID3D11Device* _device, ID3D11DeviceContext* _context) { return false; }
	void UIManager::Shutdown() { }
	void UIManager::BuildUI(Scene& _world, const Matrix& _view, const Matrix& _proj) { }
	void UIManager::Render() { }
}

#else

// ImGui Win32 백엔드의 메시지 핸들러 전방 선언(공식 예제 관례 — 헤더가 export하지 않음).
extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

namespace MiniEngine
{
	UIManager::UIManager() {}
	UIManager::~UIManager() {}

	bool UIManager::Init(HWND _hWnd, ID3D11Device* _device, ID3D11DeviceContext* _context)
	{
		IMGUI_CHECKVERSION();
		ImGui::CreateContext();
		ImGuiIO& io = ImGui::GetIO();
		io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
		ImGui::StyleColorsDark();

		if (!ImGui_ImplWin32_Init(_hWnd))
			return false;
		if (!ImGui_ImplDX11_Init(_device, _context))
			return false;

		// 기즈모 기본 오퍼레이션/모드(int 보관 멤버에 실제 enum 값 대입).
		/*
		m_gizmoOp = static_cast<int>(ImGuizmo::TRANSLATE);
		m_gizmoMode = static_cast<int>(ImGuizmo::WORLD);
		*/

		m_initialized = true;
		return true;
	}

	void UIManager::Shutdown()
	{
		if (!m_initialized)
			return;

		ImGui_ImplDX11_Shutdown();
		ImGui_ImplWin32_Shutdown();
		ImGui::DestroyContext();

		m_initialized = false;

		m_uiInsts.clear();
	}

	void UIManager::BuildUI(Scene& _world, const Matrix& _view, const Matrix& _proj)
	{
		if (!m_initialized)
			return;

		ImGui_ImplDX11_NewFrame();
		ImGui_ImplWin32_NewFrame();
		ImGui::NewFrame();
		// ImGuizmo::BeginFrame();
		// ImGuiIO& io = ImGui::GetIO();

		// 디버깅용 로그 콘솔
		//ImGui::Begin("Debug Console");
		////
		//ImGui::End();
	}

	void UIManager::Render()
	{
		if (!m_initialized)
			return;

		for (std::shared_ptr<UIBase>& ui : m_uiInsts)
			ui->Render();

		ImGui::Render();
		ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
	}

	LRESULT WndProcHandler(HWND _hWnd, UINT _msg, WPARAM _wParam, LPARAM _lParam)
	{
		if (ImGui::GetCurrentContext() == nullptr)
			return 0;
		return ImGui_ImplWin32_WndProcHandler(_hWnd, _msg, _wParam, _lParam);
	}
}

#endif