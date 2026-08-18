#include "pch.h"
#include "UI/UIBase.h"

namespace MiniEngine
{
	void UIBase::Render() 
	{
		// 위치/크기 지정은 Begin *이전*이어야 이 창에 적용된다.
		if (m_defaultPos.x > 0.0f && m_defaultPos.y > 0.0f)
			ImGui::SetNextWindowPos(m_defaultPos, ImGuiCond_FirstUseEver);

		if (m_defaultSize.x > 0.0f && m_defaultSize.y > 0.0f)
			ImGui::SetNextWindowSize(m_defaultSize, ImGuiCond_FirstUseEver);

		ImGui::Begin(m_name.c_str());
		
		DrawUI();

		ImGui::End();
	};
}
