#include "pch.h"
#include "UIBase.h"

UIBase::UIBase()
{
}

UIBase::~UIBase()
{
}

void UIBase::Update()
{
	ImGui_ImplDX12_NewFrame();
	ImGui_ImplWin32_NewFrame();

	ConstructUI(); // 하위 클래스에서 구현
	/*
	구현 예시
	ImGui::NewFrame();
	ImGui::Begin("Test GUI");
	ImGui::Text("Hello World");
	*/

	ImGui::End();
	ImGui::Render();
}
