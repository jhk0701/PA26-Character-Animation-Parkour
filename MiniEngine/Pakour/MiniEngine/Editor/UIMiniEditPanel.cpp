#include "pch.h"
#include "Editor/UIMiniEditPanel.h"

#include "Editor/AssimpBaker.h"
#include "Editor/EditorPathUtil.h"
#include "Asset/MiniLoader.h"
#include "Asset/MiniFormat.h"
#include "Asset/SkinnedMesh.h"
#include "Manager/PathManager.h"

using namespace MiniEngine::Editor;

namespace
{
    void FillBuf(char* _buf, size_t _cap, const std::string& _text)
    {
        const size_t n = (_text.size() < _cap - 1) ? _text.size() : _cap - 1;
        memcpy(_buf, _text.data(), n);
        _buf[n] = '\0';
    }
}

void UIMiniEditPanel::Construct()
{
    SetName("Edit .mini");
    m_defaultPos  = ImVec2(20.0f, 340.0f);
    m_defaultSize = ImVec2(520.0f, 360.0f);

    const std::string assets = ToUtf8(PathManager::GetInstance()->GetAssetPath());
    FillBuf(m_miniPath, sizeof(m_miniPath), assets + "Character.mini");
}

void UIMiniEditPanel::RebuildNameBufs()
{
    m_nameBufs.assign(m_clips.size(), std::array<char, 64>{});
    for (size_t i = 0; i < m_clips.size(); ++i)
        FillBuf(m_nameBufs[i].data(), m_nameBufs[i].size(), m_clips[i].name);
}

void UIMiniEditPanel::DrawUI()
{
    ImGui::InputText("Target .mini", m_miniPath, sizeof(m_miniPath));

    if (ImGui::Button("Load .mini"))
    {
        m_mesh = MiniLoader::LoadSkinnedMesh(ToWide(m_miniPath));
        if (m_mesh)
        {
            m_clips = m_mesh->GetClips(); // 편집본은 사본 — 원본 메시는 건드리지 않는다
            RebuildNameBufs();
            m_dirty = false;
            m_msg   = "loaded: " + std::to_string(m_clips.size()) + " clips, "
                    + std::to_string(m_mesh->GetSkeleton().bones.size()) + " bones";
        }
        else
        {
            m_clips.clear();
            m_nameBufs.clear();
            m_msg = "load failed (SkinnedMesh .mini 가 맞는지 확인)";
        }
    }

    if (!m_mesh)
    {
        if (!m_msg.empty())
            ImGui::TextWrapped("%s", m_msg.c_str());
        return;
    }

    // 축 정규화 이전에 구워진 애셋이면 리타게팅/루트모션 전제가 깨져 있다.
    if ((m_mesh->GetBakeFlags() & MINI_BAKE_AXIS_NORMALIZED) == 0)
        ImGui::TextColored(ImVec4(1.0f, 0.7f, 0.3f, 1.0f),
                           "stale asset: axis not normalized - Baker 로 재베이크 권장");

    ImGui::Separator();
    ImGui::Text("Clips (%d)", static_cast<int>(m_clips.size()));

    // 순회 중 벡터를 변형하지 않는다 — 변형 요청은 모아서 루프 뒤에 한 번만 적용.
    enum class Op { None, Up, Down, Erase };
    Op  pendingOp    = Op::None;
    int pendingIndex = -1;

    for (int i = 0; i < static_cast<int>(m_clips.size()); ++i)
    {
        ImGui::PushID(i);

        ImGui::SetNextItemWidth(240.0f);
        if (ImGui::InputText("##name", m_nameBufs[i].data(), m_nameBufs[i].size()))
        {
            m_clips[i].name = m_nameBufs[i].data();
            m_dirty = true;
        }

        ImGui::SameLine();
        if (ImGui::SmallButton("Up"))   { pendingOp = Op::Up;    pendingIndex = i; }
        ImGui::SameLine();
        if (ImGui::SmallButton("Dn"))   { pendingOp = Op::Down;  pendingIndex = i; }
        ImGui::SameLine();
        if (ImGui::SmallButton("X"))    { pendingOp = Op::Erase; pendingIndex = i; }
        ImGui::SameLine();
        ImGui::TextDisabled("#%d  %.2fs", i, m_clips[i].ClipDurationSec());

        ImGui::PopID();
    }

    if (m_clips.empty())
        ImGui::TextDisabled("(no clips)");

    switch (pendingOp)
    {
    case Op::Up:
        if (pendingIndex > 0)
        {
            std::swap(m_clips[pendingIndex], m_clips[pendingIndex - 1]);
            RebuildNameBufs();
            m_dirty = true;
        }
        break;
    case Op::Down:
        if (pendingIndex >= 0 && pendingIndex + 1 < static_cast<int>(m_clips.size()))
        {
            std::swap(m_clips[pendingIndex], m_clips[pendingIndex + 1]);
            RebuildNameBufs();
            m_dirty = true;
        }
        break;
    case Op::Erase:
        if (pendingIndex >= 0 && pendingIndex < static_cast<int>(m_clips.size()))
        {
            m_clips.erase(m_clips.begin() + pendingIndex);
            RebuildNameBufs();
            m_dirty = true;
        }
        break;
    default:
        break;
    }

    ImGui::Separator();

    // ── 애니 소스 리타게팅 후 덧붙이기 ──
    ImGui::Combo("Up Axis##edit", &m_upAxis, "Auto\0Y-up\0Z-up\0-Z-up\0");
    ImGui::InputText("Append anims (;)", m_appendAnims, sizeof(m_appendAnims));

    if (ImGui::Button("Append"))
    {
        const std::vector<std::wstring> srcs = SplitPaths(m_appendAnims);
        if (srcs.empty())
        {
            m_msg = "append: 소스 경로가 비어 있다";
        }
        else
        {
            const size_t before = m_clips.size();
            BakeOptions opt;
            opt.upAxis = static_cast<BakeUpAxis>(m_upAxis);

            const BakeResult r = AssimpBaker::RetargetAnims(m_mesh->GetSkeleton(), srcs, m_clips, opt);
            if (r.success)
            {
                RebuildNameBufs();
                m_dirty = true;
                m_msg   = "appended " + std::to_string(m_clips.size() - before) + " clip(s)";
            }
            else
            {
                m_msg = r.message;
            }
        }
    }

    ImGui::Separator();

    if (ImGui::Button("Save"))
    {
        const bool ok = MiniLoader::WriteSkinnedMesh(ToWide(m_miniPath),
                                                     m_mesh->GetVertices(),
                                                     m_mesh->GetIndices(),
                                                     m_mesh->GetSkeleton(),
                                                     m_clips,
                                                     m_mesh->GetBakeFlags());
        m_dirty = !ok;
        m_msg   = ok ? ("saved: " + std::to_string(m_clips.size()) + " clips")
                     : std::string("save failed");
    }

    if (m_dirty)
    {
        ImGui::SameLine();
        ImGui::TextColored(ImVec4(1.0f, 0.8f, 0.3f, 1.0f), "(unsaved)");
    }

    if (!m_msg.empty())
        ImGui::TextWrapped("%s", m_msg.c_str());
}
