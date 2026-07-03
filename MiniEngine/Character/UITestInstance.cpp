#include "pch.h"
#include "UITestInstance.h"

void UITestInstance::ConstructUI()
{
	ImGui::NewFrame();
	ImGui::Begin("Test GUI");
	ImGui::Text(curText.c_str());
}
