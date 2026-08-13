#include "pch.h"
#include "Content/UI/UIDebugPerceptionResult.h"

#include "Perception/PerceptionComponent.h"
#include "Perception/ProcessorComponent.h"
#include "Content/Character.h"

void UIDebugPerceptionResult::Construct()
{
	SetName("Perception Result");
}

void UIDebugPerceptionResult::DrawUI()
{
	if (m_pChar.expired())
		return;

	std::shared_ptr<Character> pChar = m_pChar.lock();

	ImGui::SetNextWindowSize(ImVec2(400.0f, 400.0f), ImGuiCond_FirstUseEver);

	ImGui::Text("Character State : %s", pChar->GetStateName((uint8_t)pChar->GetState()));
	
	const std::vector<std::shared_ptr<ProcessCondition>>& conditions = pChar->GetProcessor().lock()->GetConditions();
	ImGui::Text("Process Conditions (%d)", conditions.size());

	int i = 0;
	for (const std::shared_ptr<ProcessCondition>& cond : conditions)
	{
		ImGui::Text("[#(%d)] %s :: %s", i, cond->GetName().c_str(), cond->GetResult() ? "true" : "false");
		i++;
	}
}
