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

#ifdef MG_DEBUG_UI

void UIDebugPerceptionResult::DrawUI()
{

	if (m_pChar.expired())
		return;

	ImGui::SetNextWindowSize(ImVec2(400.0f, 600.0f), ImGuiCond_FirstUseEver);

	std::shared_ptr<Character> pChar = m_pChar.lock();
	ImGui::Text("Character State : %s", pChar->GetStateName((uint8_t)pChar->GetState()));

	if (ImGui::CollapsingHeader("Perceptions"))
	{
		Actor* pCurObs = dynamic_cast<Actor*>(pChar->GetCurObstacle());
		ImGui::Text("Current Obs Name : %s", pCurObs != nullptr ? pCurObs->GetName().c_str() : "NULL");

		const PerceptedObstacleInfo& INFO = pChar->GetCurObstacleInfo();
		Actor* pPerceptedObs = dynamic_cast<Actor*>(INFO.perceptResult.pObstacle);
		ImGui::Text("Percepted Obs Name : %s", pPerceptedObs != nullptr ? pPerceptedObs->GetName().c_str() : "NULL");

		ImGui::Text("Height : %.3f, Depth : %.3f, Dist : %.3f", INFO.obstacleHeight, INFO.perceptResult.obstacleDepth, INFO.perceptResult.obstacleDistance);

		if (fabs(INFO.perceptResult.roomHeight - FLT_MAX) < 1e-4f)
			ImGui::Text("Room : FLT_MAX");
		else 
			ImGui::Text("Room : %.3f", INFO.perceptResult.roomHeight);

		ImGui::Text("Character Falling Time(sec) : %.2f", pChar->GetFallingTime());
	}

	if (ImGui::BeginChild("Processor Results", ImVec2(400, 600), ImGuiChildFlags_Borders))
	{
		if (ImGui::CollapsingHeader("Conditions"))
		{
			const std::vector<std::shared_ptr<ProcessCondition>>& conditions = pChar->GetProcessor().lock()->GetConditions(pChar->GetCurPerceptType());
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
			const std::vector<std::shared_ptr<ProcessData>>& processDatas = pChar->GetProcessor().lock()->GetProcessDatas(pChar->GetCurPerceptType());
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

#else
void UIDebugPerceptionResult::DrawUI() {}
#endif // MG_DEBUG_UI

