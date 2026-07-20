#include "pch.h"
#include "UI/UIBase.h"

namespace MiniEngine
{
	void UIBase::Render() 
	{
		ImGui::Begin(m_name.c_str());
		
		DrawUI();

		ImGui::End();
	};
}