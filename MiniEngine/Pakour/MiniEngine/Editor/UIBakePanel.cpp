#include "pch.h"
#include "Editor/UIBakePanel.h"

#include "Editor/AssimpBaker.h"
#include "Editor/EditorPathUtil.h"
#include "Manager/PathManager.h"

using namespace MiniEngine::Editor;

namespace
{
    // 버퍼에 UTF-8 문자열을 안전하게 채운다(널 종단 보장).
    void FillBuf(char* _buf, size_t _cap, const std::string& _text)
    {
        const size_t n = (_text.size() < _cap - 1) ? _text.size() : _cap - 1;
        memcpy(_buf, _text.data(), n);
        _buf[n] = '\0';
    }
}

void UIBakePanel::Construct()
{
    SetName("Baker");
    m_defaultPos  = ImVec2(20.0f, 20.0f);
    m_defaultSize = ImVec2(520.0f, 300.0f);

    // 기본 경로는 exe 기준 Assets 폴더. 실제 파일명은 사용자가 채운다.
    const std::string assets = ToUtf8(PathManager::GetInstance()->GetAssetPath());
    FillBuf(m_src,      sizeof(m_src),      assets + "Character.fbx");
    FillBuf(m_out,      sizeof(m_out),      assets + "Character.mini");
    FillBuf(m_animsDir, sizeof(m_animsDir), assets + "Motion");
}

void UIBakePanel::DrawUI()
{
    ImGui::TextDisabled("Source model -> .mini");

    ImGui::InputText("Source", m_src, sizeof(m_src));
    ImGui::InputText("Output", m_out, sizeof(m_out));

    // 추가 애니 소스(호환 스켈레톤 FBX, ';' 구분) — 클립을 같은 .mini 에 병합.
    ImGui::InputText("Anims (extra, ;)", m_anims, sizeof(m_anims));

    ImGui::InputText("Anims folder", m_animsDir, sizeof(m_animsDir));
    if (ImGui::IsItemHovered())
        ImGui::SetTooltip(u8"폴더 안의 .fbx/.gltf/.glb/.dae 를 비재귀로 전부 추가한다.\n"
                          u8"Source 자기 자신은 제외되고, 이름 순으로 정렬돼 클립 순서가 결정적이다.");

    // BakeOptions — 베이커 코어는 지원하는데 그동안 UI 가 없어 못 쓰던 값들.
    ImGui::Combo("Up Axis", &m_upAxis, "Auto\0Y-up\0Z-up\0-Z-up\0");
    if (ImGui::IsItemHovered())
        ImGui::SetTooltip(u8"베이크 공간의 up 축. Auto = assimp 축 변환을 신뢰(정상 FBX 전부).");

    ImGui::Combo("Forward Axis", &m_forwardAxis, "Auto\0+Z\0-Z\0+X\0-X\0");
    if (ImGui::IsItemHovered())
        ImGui::SetTooltip(u8"캐릭터 정면. Auto = rest 에서 실측해 +Z 로 세운다(90도 단위 스냅).");

    ImGui::Separator();

    if (ImGui::Button("Bake"))
    {
        const std::wstring src = ToWide(m_src);
        const std::wstring out = ToWide(m_out);

        std::vector<std::wstring> extras = SplitPaths(m_anims);
        const std::vector<std::wstring> folder = EnumerateAnimFolder(m_animsDir, src);
        extras.insert(extras.end(), folder.begin(), folder.end());

        const BakeOptions opt{ static_cast<BakeUpAxis>(m_upAxis),
                               static_cast<BakeForwardAxis>(m_forwardAxis) };

        const BakeResult r = AssimpBaker::Bake(src, out, extras, opt); // 본 유무로 Static/Skinned 자동 감지
        m_lastOk      = r.success;
        m_lastSkinned = r.skinned;
        m_lastVerts   = static_cast<int>(r.vertexCount);
        m_lastIdx     = static_cast<int>(r.indexCount);
        m_lastBones   = static_cast<int>(r.boneCount);
        m_lastClips   = static_cast<int>(r.clipCount);
        m_lastMsg     = r.message;
        m_lastUp      = r.detectedUp;
        m_lastForward = r.detectedForward;
    }

    if (m_lastMsg.empty())
        return;

    if (m_lastOk && m_lastSkinned)
        ImGui::Text("Last: %d verts, %d idx, %d bones, %d clips (skinned)",
                    m_lastVerts, m_lastIdx, m_lastBones, m_lastClips);
    else if (m_lastOk)
        ImGui::Text("Last: %d verts, %d idx (static)", m_lastVerts, m_lastIdx);
    else
        ImGui::TextColored(ImVec4(1.0f, 0.4f, 0.4f, 1.0f), "Last: %s", m_lastMsg.c_str());

    if (!m_lastUp.empty())
        ImGui::TextDisabled("detected up (pre-normalize): %s", m_lastUp.c_str());
    if (!m_lastForward.empty())
        ImGui::TextDisabled("detected forward (after up-fix): %s", m_lastForward.c_str());
}
