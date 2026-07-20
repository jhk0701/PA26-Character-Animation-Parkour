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

	private:
		std::string m_name;
	};
}
