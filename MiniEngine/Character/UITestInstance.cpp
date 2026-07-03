#include "pch.h"
#include "UITestInstance.h"

void UITestInstance::ConstructUI()
{
	ImGui::NewFrame();
	ImGui::Begin("Test GUI");
	ImGui::Text(m_curText.c_str());

	if (m_pBlendVal)
		ImGui::SliderFloat("Blend", m_pBlendVal, 0, 1);
}
