#include "pch.h"
#include "Content/UI/UIDebugPerceptionResult.h"

#include "Perception/PerceptionComponent.h"
#include "Perception/ProcessorComponent.h"
#include "Content/Character.h"

using namespace MiniEngine;

void UIDebugPerceptionResult::Construct()
{
	SetName("Perception Result");
}

void UIDebugPerceptionResult::DrawUI()
{
	if (m_pChar.expired())
		return;

	std::shared_ptr<Character> pChar = m_pChar.lock();

	ImGui::SetNextWindowSize(ImVec2(400.0f, 600.0f), ImGuiCond_FirstUseEver);

	ImGui::Text("Character State : %s", pChar->GetStateName((uint8_t)pChar->GetState()));
	
	Actor* pCurObs = dynamic_cast<Actor*>(pChar->GetCurObstacle());
	ImGui::Text("Current Obs Name : %s", pCurObs != nullptr ? pCurObs->GetName().c_str() : "NULL");

	Actor* pPerceptedObs = dynamic_cast<Actor*>(pChar->GetCurObstacleInfo().m_pObstacle);
	ImGui::Text("Percepted Obs Name : %s", pPerceptedObs != nullptr ? pPerceptedObs->GetName().c_str() : "NULL");

	if (ImGui::BeginChild("Processor Results", ImVec2(400, 600), ImGuiChildFlags_Borders)) 
	{
		if (ImGui::CollapsingHeader("Conditions"))
		{
			const std::vector<std::shared_ptr<ProcessCondition>>& conditions = pChar->GetProcessor().lock()->GetConditions();
			ImGui::Text("Conditions (%d)", conditions.size());

			if (ImGui::BeginListBox("##Condition List", ImVec2(350, 600)))
			{
				int i = 0;
				for (const std::shared_ptr<ProcessCondition>& cond : conditions)
				{
					const ImVec4 COL = cond->GetResult() ? ImVec4(0.0f, 1.0f, 0.0f, 1.0f) : ImVec4(1.0f, 0.0f, 0.0f, 1.0f);
					ImGui::TextColored(COL,
						"[#%d] %s : %s", i, cond->GetName().c_str(), cond->GetResult() ? "true" : "false");
					i++;
				}

				ImGui::EndListBox();
			}
		}

		if (ImGui::CollapsingHeader("Process Data"))
		{
			const std::vector<std::shared_ptr<ProcessData>>& processDatas = pChar->GetProcessor().lock()->GetProcessDatas();
			ImGui::Text("Process Datas (%d)", processDatas.size());
			if (ImGui::BeginListBox("##Process Datas List", ImVec2(350, 600)))
			{
				int i = 0;
				for (const std::shared_ptr<ProcessData>& data : processDatas)
				{
					const bool RESULT = data->GetCondition()->GetResult();
					const ImVec4 COL = RESULT ? ImVec4(0.0f, 1.0f, 0.0f, 1.0f) : ImVec4(1.0f, 0.0f, 0.0f, 1.0f);

					ImGui::TextColored(COL,
						"[#%d] %s : %s", i, data->GetName().c_str(), RESULT ? "true" : "false");
					i++;
				}

				ImGui::EndListBox();
			}
		}
	}
	ImGui::EndChild();
}
