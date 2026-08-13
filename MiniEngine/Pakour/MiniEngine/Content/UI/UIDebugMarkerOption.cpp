#include "pch.h"
#include "Content/UI/UIDebugMarkerOption.h"

#include "Scene/Scene.h"

void UIDebugMarkerOption::Construct()
{
	SetName("Debugger Drawer Toggle");
}

void UIDebugMarkerOption::DrawUI()
{
	if (m_pScene.expired())
		return;

	ImGui::SetNextWindowSize(ImVec2(200.0f, 100.0f), ImGuiCond_FirstUseEver);

	if (ImGui::Checkbox("Apply Physics Drawer", &m_bApplyPhysic)) 
		m_pScene.lock()->ApplyPhysicsDebug(m_bApplyPhysic);

	if (ImGui::Checkbox("Apply Marker Drawer", &m_bApplyMarker)) 
		m_pScene.lock()->ApplyMarkerDebug(m_bApplyMarker);
}
