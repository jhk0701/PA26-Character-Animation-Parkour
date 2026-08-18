#include "pch.h"
#include "Content/UI/UIDebugBlendClip.h"
#include "Content/Character.h"

#include "Animation/Animator.h"
#include "Animation/AnimStateMachine.h"
#include "Animation/BlendClip.h"
#include "Scene/SkeletalMeshComponent.h"

using namespace MiniEngine;

void UIDebugBlendClip::Construct()
{
	SetName("Debug Blend Space");
	m_defaultSize = ImVec2(400.0f, 400.0f);
}

void UIDebugBlendClip::DrawUI()
{
	if (m_pChar.expired())
		return;


	std::shared_ptr<Character> pChar = m_pChar.lock();
	std::shared_ptr<Animator> pAnim = pChar->GetSkin().lock()->GetAnim().lock();

	ImGui::Text("%s", m_pChar.lock()->GetName().c_str());
	
	DrawGraph(pAnim);
}

void UIDebugBlendClip::DrawGraph(std::shared_ptr<MiniEngine::Animator>& _pAnim)
{
	// 그래프용 분리
	ImGui::Separator();
	const float SIZE = min(360.0f, max(180.0f, ImGui::GetContentRegionAvail().x - 8.0f));

	AnimStateMachine& sm = _pAnim->GetBaseTrack();
	std::shared_ptr<BlendClip> pClip = sm.GetClip(0);

	const float px = pClip->GetAxisX().m_val;
	const float py = pClip->GetAxisY().m_val;
	ImGui::Text("x : %.3f, y : %.3f", px, py);

	ImDrawList* dl = ImGui::GetWindowDrawList();
	const ImVec2 org = ImGui::GetCursorScreenPos();

	float minx = px, maxx = px;
	float miny = py, maxy = py;

	const auto& PLACEMENTS = pClip->GetPlacement();
	for (const auto& place : PLACEMENTS)
	{
		minx = min(minx, place.m_coord.x);
		maxx = max(maxx, place.m_coord.x);
		miny = min(miny, place.m_coord.y);
		maxy = max(maxy, place.m_coord.y);
	}
	const float padx = (maxx - minx) * 0.15f + 1e-3f;
	const float pady = (maxy - miny) * 0.15f + 1e-3f;
	minx -= padx; maxx += padx; miny -= pady; maxy += pady;

	auto toScreen = [&](float _x, float _y) {
		return ImVec2(org.x + (_x - minx) / (maxx - minx) * SIZE,
			org.y + SIZE - (_y - miny) / (maxy - miny) * SIZE);
	};

	dl->AddRectFilled(org, ImVec2(org.x + SIZE, org.y + SIZE), IM_COL32(28, 28, 32, 255));
	dl->AddRect(org, ImVec2(org.x + SIZE, org.y + SIZE), IM_COL32(90, 90, 100, 255));

	if (pClip) 
	{
		for (const auto& tri : pClip->GetTris())
		{
			const ImVec2 a = toScreen(PLACEMENTS[tri.i0].m_coord.x, PLACEMENTS[tri.i0].m_coord.y);
			const ImVec2 b = toScreen(PLACEMENTS[tri.i1].m_coord.x, PLACEMENTS[tri.i1].m_coord.y);
			const ImVec2 c = toScreen(PLACEMENTS[tri.i2].m_coord.x, PLACEMENTS[tri.i2].m_coord.y);
			dl->AddTriangle(a, b, c, IM_COL32(80, 140, 200, 200), 1.5f);
		}
	}

	// 점 표시
	for (const auto& pl : PLACEMENTS)
	{
		float w = pl.m_weight;

		const ImVec2 p = toScreen(pl.m_coord.x, pl.m_coord.y);
		const ImU32 col = (w > 0.0f ? IM_COL32(255, 190, 60, 255) : IM_COL32(140, 140, 150, 255));
		dl->AddCircleFilled(p, 3.0f + 7.0f * w, col);

		if (w > 0.0f)
		{
			char buf[16];
			snprintf(buf, sizeof(buf), "%.0f%%", w * 100.0f);
			dl->AddText(ImVec2(p.x + 9.0f, p.y - 7.0f), IM_COL32(255, 220, 140, 255), buf);
		}
	}

	const ImVec2 pp = toScreen(px, py);
	dl->AddLine(ImVec2(pp.x - 7, pp.y), ImVec2(pp.x + 7, pp.y), IM_COL32(80, 255, 120, 255), 1.5f);
	dl->AddLine(ImVec2(pp.x, pp.y - 7), ImVec2(pp.x, pp.y + 7), IM_COL32(80, 255, 120, 255), 1.5f);

	ImGui::Separator();

	// 기여도 표시
	float wSum = 0.0f;
	for (const auto& pl : PLACEMENTS)
	{
		wSum += pl.m_weight;
		const char* name = pl.m_pClip->name.c_str();
		ImGui::TextColored(ImVec4(1.0f, 0.78f, 0.35f, 1.0f), "  %5.1f%%  %s", pl.m_weight * 100.0f, name);
	}
}
