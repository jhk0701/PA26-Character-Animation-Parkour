#pragma once

#include <imgui.h>
#include <imgui_impl_win32.h>
#include <imgui_impl_dx11.h>

namespace MiniEngine 
{
	class UIBase
	{
	public:
		virtual ~UIBase() {};
		virtual void Construct() = 0;
		void Render();

		void SetName(const std::string& _newName) { m_name = _newName; }
		const std::string& GetName() const { return m_name; }

	protected:
		virtual void DrawUI() = 0;

		// 첫 표시 시 창 위치/크기(0 이면 미지정 = ImGui 자동). Construct 에서 세팅한다.
		// FirstUseEver 라 imgui.ini 에 저장된 사용자 배치가 있으면 그쪽이 이긴다.
		// ⚠ DrawUI 안에서 SetNextWindowSize 를 부르면 Begin 이후라 *다음* 창에 적용된다.
		ImVec2 m_defaultPos{ 0.0f, 0.0f };
		ImVec2 m_defaultSize{ 0.0f, 0.0f };

	private:
		std::string m_name;
	};
}
