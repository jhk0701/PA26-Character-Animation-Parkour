#include "pch.h"
#include "Editor/EditorUI.h"

#if defined(WITH_EDITOR)
// ─── Editor 구성: 실제 ImGui 구현 ───────────────────────────────────────────
#include "Scene/Scene.h"
#include "Scene/Actor.h"
#include "Scene/SceneComponent.h"
#include "Scene/SkeletalMeshComponent.h" // Inspector Animation 섹션(전 구성 타입 — 격리 무영향)
#include "Core/Math.h"
#include "Editor/AssimpBaker.h" // Baker 패널 → 원본 모델 .mini 베이크

#include <imgui.h>
#include <imgui_impl_win32.h>
#include <imgui_impl_dx11.h>
#include <ImGuizmo.h>

// ImGui Win32 백엔드의 메시지 핸들러 전방 선언(공식 예제 관례 — 헤더가 export하지 않음).
extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

#include <string>
#include <vector>
#include <typeinfo>

namespace
{
    // UTF-8 char 버퍼(ImGui InputText) → wstring 경로.
    std::wstring ToWide(const char* _utf8)
    {
        if (!_utf8 || !*_utf8) return {};
        const int len = ::MultiByteToWideChar(CP_UTF8, 0, _utf8, -1, nullptr, 0);
        if (len <= 0) return {};
        std::wstring out(static_cast<size_t>(len - 1), L'\0'); // len 은 널 종단 포함
        ::MultiByteToWideChar(CP_UTF8, 0, _utf8, -1, out.data(), len);
        return out;
    }

    // ';' 구분 다중 경로 입력 → wstring 목록(앞뒤 공백 트림, 빈 항목 스킵).
    std::vector<std::wstring> SplitPaths(const char* _utf8)
    {
        std::vector<std::wstring> out;
        if (!_utf8) return out;
        std::string buf(_utf8);
        size_t begin = 0;
        while (begin <= buf.size())
        {
            size_t end = buf.find(';', begin);
            if (end == std::string::npos) end = buf.size();
            std::string item = buf.substr(begin, end - begin);
            const size_t first = item.find_first_not_of(" \t");
            const size_t last  = item.find_last_not_of(" \t");
            if (first != std::string::npos)
            {
                item = item.substr(first, last - first + 1);
                out.push_back(ToWide(item.c_str()));
            }
            begin = end + 1;
        }
        return out;
    }
}

namespace MiniEngine
{
    namespace Editor
    {
        bool EditorUI::Initialize(HWND _hWnd, ID3D11Device* _device, ID3D11DeviceContext* _context)
        {
            IMGUI_CHECKVERSION();
            ImGui::CreateContext();
            ImGuiIO& io = ImGui::GetIO();
            io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
            ImGui::StyleColorsDark();

            if (!ImGui_ImplWin32_Init(_hWnd))
                return false;
            if (!ImGui_ImplDX11_Init(_device, _context))
                return false;

            // 기즈모 기본 오퍼레이션/모드(int 보관 멤버에 실제 enum 값 대입).
            m_gizmoOp   = static_cast<int>(ImGuizmo::TRANSLATE);
            m_gizmoMode = static_cast<int>(ImGuizmo::WORLD);

            m_initialized = true;
            return true;
        }

        void EditorUI::Shutdown()
        {
            if (!m_initialized)
                return;
            ImGui_ImplDX11_Shutdown();
            ImGui_ImplWin32_Shutdown();
            ImGui::DestroyContext();
            m_initialized = false;
        }

        void EditorUI::BuildUI(Scene& _world, const Matrix& _view, const Matrix& _proj)
        {
            if (!m_initialized)
                return;

            ImGui_ImplDX11_NewFrame();
            ImGui_ImplWin32_NewFrame();
            ImGui::NewFrame();
            ImGuizmo::BeginFrame();

            ImGuiIO& io = ImGui::GetIO();
            // 텍스트 입력 중이 아닐 때 W/E/R 로 오퍼레이션 전환(이동/회전/스케일).
            if (!io.WantTextInput)
            {
                if (ImGui::IsKeyPressed(ImGuiKey_W)) m_gizmoOp = static_cast<int>(ImGuizmo::TRANSLATE);
                if (ImGui::IsKeyPressed(ImGuiKey_E)) m_gizmoOp = static_cast<int>(ImGuizmo::ROTATE);
                if (ImGui::IsKeyPressed(ImGuiKey_R)) m_gizmoOp = static_cast<int>(ImGuizmo::SCALE);
            }

            const auto& actors = _world.GetActors();
            const int actorCount = static_cast<int>(actors.size());

            // ── Hierarchy: 월드의 Actor 목록 + 선택 ──
            ImGui::Begin("Hierarchy");
            for (int i = 0; i < actorCount; ++i)
            {
                const std::string label = actors[i]->GetName() + "##" + std::to_string(i);
                if (ImGui::Selectable(label.c_str(), m_selectedIndex == i))
                    m_selectedIndex = i;
            }
            if (actorCount == 0)
                ImGui::TextDisabled("(no actors)");
            ImGui::End();

            // ── Inspector: 선택 Actor의 root transform 편집 + 컴포넌트 목록 ──
            ImGui::Begin("Inspector");
            if (m_selectedIndex >= 0 && m_selectedIndex < actorCount)
            {
                const auto& actor = actors[m_selectedIndex];
                ImGui::Text("%s", actor->GetName().c_str());
                ImGui::Separator();

                if (auto root = actor->GetRoot())
                {
                    Transform& t = root->localTransform;

                    float pos[3] = { t.position.x, t.position.y, t.position.z };
                    if (ImGui::DragFloat3("Position", pos, 0.05f))
                        t.position = Vector3(pos[0], pos[1], pos[2]);

                    // 회전: 쿼터니언 ↔ 오일러(도) 변환. ToEuler()=(pitch=x, yaw=y, roll=z) 라디안.
                    const Vector3 euler = t.rotation.ToEuler();
                    float rot[3] = { ToDegrees(euler.x), ToDegrees(euler.y), ToDegrees(euler.z) };
                    if (ImGui::DragFloat3("Rotation", rot, 0.5f))
                        t.rotation = Quaternion::CreateFromYawPitchRoll(
                            ToRadians(rot[1]), ToRadians(rot[0]), ToRadians(rot[2]));

                    float scl[3] = { t.scale.x, t.scale.y, t.scale.z };
                    if (ImGui::DragFloat3("Scale", scl, 0.05f))
                        t.scale = Vector3(scl[0], scl[1], scl[2]);

                    // ── 기즈모 오퍼레이션/모드 선택 (W/E/R 단축과 동기) ──
                    ImGui::Separator();
                    ImGui::TextDisabled("Gizmo (W/E/R)");
                    if (ImGui::RadioButton("Translate", m_gizmoOp == static_cast<int>(ImGuizmo::TRANSLATE))) m_gizmoOp = static_cast<int>(ImGuizmo::TRANSLATE);
                    ImGui::SameLine();
                    if (ImGui::RadioButton("Rotate",    m_gizmoOp == static_cast<int>(ImGuizmo::ROTATE)))    m_gizmoOp = static_cast<int>(ImGuizmo::ROTATE);
                    ImGui::SameLine();
                    if (ImGui::RadioButton("Scale",     m_gizmoOp == static_cast<int>(ImGuizmo::SCALE)))     m_gizmoOp = static_cast<int>(ImGuizmo::SCALE);
                    if (ImGui::RadioButton("Local", m_gizmoMode == static_cast<int>(ImGuizmo::LOCAL))) m_gizmoMode = static_cast<int>(ImGuizmo::LOCAL);
                    ImGui::SameLine();
                    if (ImGui::RadioButton("World", m_gizmoMode == static_cast<int>(ImGuizmo::WORLD))) m_gizmoMode = static_cast<int>(ImGuizmo::WORLD);

                    // ── 3D 뷰포트 기즈모: root 월드행렬 조작 → 로컬로 역산 대입 ──
                    ImGuizmo::SetOrthographic(false);
                    ImGuizmo::SetDrawlist(ImGui::GetForegroundDrawList()); // 씬 위(foreground)에 그림
                    ImGuizmo::SetRect(0.0f, 0.0f, io.DisplaySize.x, io.DisplaySize.y);

                    Matrix world = root->GetWorldMatrix();
                    if (ImGuizmo::Manipulate(&_view._11, &_proj._11,
                            static_cast<ImGuizmo::OPERATION>(m_gizmoOp),
                            static_cast<ImGuizmo::MODE>(m_gizmoMode),
                            &world._11))
                    {
                        // world = local * parentWorld  →  local = world * parentWorld^-1.
                        Matrix local = world;
                        if (auto parent = root->GetParent())
                            local = world * parent->GetWorldMatrix().Invert();

                        Vector3 s, tr; Quaternion r;
                        if (local.Decompose(s, r, tr))
                        {
                            t.position = tr;
                            t.rotation = r;
                            t.scale    = s;
                        }
                    }
                }
                else
                {
                    ImGui::TextDisabled("(no root transform)");
                }

                // ── Animation: SkeletalMeshComponent 클립 목록 + 크로스페이드 재생 ──
                if (auto skeletal = actor->GetComponent<SkeletalMeshComponent>())
                {
                    if (skeletal->GetMesh().expired() == false)
                    {
                        ImGui::Separator();
                        ImGui::Text("Animation");
                        ImGui::DragFloat("Fade (s)", &m_animFadeSec, 0.05f, 0.0f, 10.0f);

                        const auto& clips = skeletal->GetMesh().lock()->GetClips();
                        for (int c = 0; c < static_cast<int>(clips.size()); ++c)
                        {
                            const std::string label = clips[c].name + "##clip" + std::to_string(c);
                            if (ImGui::Selectable(label.c_str(), skeletal->GetActiveClip() == c))
                                skeletal->PlayClip(c, m_animFadeSec);
                        }
                        if (clips.empty())
                            ImGui::TextDisabled("(no clips)");

                        if (ImGui::Button("Stop (Bind Pose)"))
                            skeletal->PlayClip(-1, m_animFadeSec);
                        if (skeletal->IsFading())
                        {
                            ImGui::SameLine();
                            ImGui::TextDisabled("(fading...)");
                        }
                    }
                }

                ImGui::Separator();
                ImGui::Text("Components:");
                for (const auto& comp : actor->GetComponents())
                    ImGui::BulletText("%s", typeid(*comp).name());
            }
            else
            {
                ImGui::TextDisabled("No actor selected.");
            }
            ImGui::End();

            // ── Baker: 원본 모델(.fbx/.gltf/.obj) → .mini 베이크 (+ 씬 로드) ──
            ImGui::Begin("Baker");
            ImGui::TextDisabled("Source model -> .mini");
            ImGui::InputText("Source", m_bakerSrc, sizeof(m_bakerSrc));
            ImGui::InputText("Output", m_bakerOut, sizeof(m_bakerOut));
            // 추가 애니 소스(동일 스켈레톤 FBX, ';' 구분) — 클립을 같은 .mini 에 병합.
            ImGui::InputText("Anims (extra, ;)", m_bakerAnims, sizeof(m_bakerAnims));

            const bool bake     = ImGui::Button("Bake");
            ImGui::SameLine();
            const bool bakeLoad = ImGui::Button("Bake & Load");
            if (bake || bakeLoad)
            {
                const std::wstring src = ToWide(m_bakerSrc);
                const std::wstring out = ToWide(m_bakerOut);
                const std::vector<std::wstring> extras = SplitPaths(m_bakerAnims);
                const BakeResult r = AssimpBaker::Bake(src, out, extras); // 본 유무로 Static/Skinned 자동 감지
                m_lastBakeOk      = r.success;
                m_lastBakeSkinned = r.skinned;
                m_lastBakeVerts   = static_cast<int>(r.vertexCount);
                m_lastBakeIdx     = static_cast<int>(r.indexCount);
                m_lastBakeBones   = static_cast<int>(r.boneCount);
                m_lastBakeClips   = static_cast<int>(r.clipCount);
                m_lastBakeMsg     = r.message;
                if (r.success && bakeLoad)
                    m_pendingLoadMini = out; // GameCore 가 소비해 씬에 스폰
            }

            if (!m_lastBakeMsg.empty())
            {
                if (m_lastBakeOk && m_lastBakeSkinned)
                    ImGui::Text("Last: %d verts, %d idx, %d bones, %d clips (skinned)",
                                m_lastBakeVerts, m_lastBakeIdx, m_lastBakeBones, m_lastBakeClips);
                else if (m_lastBakeOk)
                    ImGui::Text("Last: %d verts, %d idx (static)", m_lastBakeVerts, m_lastBakeIdx);
                else
                    ImGui::TextColored(ImVec4(1, 0.4f, 0.4f, 1), "Last: %s", m_lastBakeMsg.c_str());
            }
            ImGui::End();
        }

        std::wstring EditorUI::ConsumePendingLoadMini()
        {
            std::wstring path = std::move(m_pendingLoadMini);
            m_pendingLoadMini.clear();
            return path;
        }

        void EditorUI::Render()
        {
            if (!m_initialized)
                return;

            ImGui::Render();
            ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
        }

        bool EditorUI::WantCaptureMouse() const
        {
            if (!m_initialized)
                return false;
            return ImGui::GetIO().WantCaptureMouse;
        }

        bool EditorUI::WantCaptureKeyboard() const
        {
            if (!m_initialized)
                return false;
            return ImGui::GetIO().WantCaptureKeyboard;
        }

        bool EditorUI::IsGizmoActive() const
        {
            if (!m_initialized)
                return false;
            return ImGuizmo::IsOver() || ImGuizmo::IsUsing();
        }

        LRESULT WndProcHandler(HWND _hWnd, UINT _msg, WPARAM _wParam, LPARAM _lParam)
        {
            if (ImGui::GetCurrentContext() == nullptr)
                return 0;
            return ImGui_ImplWin32_WndProcHandler(_hWnd, _msg, _wParam, _lParam);
        }
    }
}

#else

namespace MiniEngine
{
    class Scene;
    namespace Editor
    {
        bool EditorUI::Initialize(HWND, ID3D11Device*, ID3D11DeviceContext*) { m_initialized = false; return true; }
        void EditorUI::Shutdown() {}
        void EditorUI::BuildUI(Scene&, const Matrix&, const Matrix&) {}
        void EditorUI::Render() {}
        bool EditorUI::WantCaptureMouse() const { return false; }
        bool EditorUI::WantCaptureKeyboard() const { return false; }
        bool EditorUI::IsGizmoActive() const { return false; }
        std::wstring EditorUI::ConsumePendingLoadMini() { return {}; }
        LRESULT WndProcHandler(HWND, UINT, WPARAM, LPARAM) { return 0; }
    }
}
#endif
