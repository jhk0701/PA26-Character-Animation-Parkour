#pragma once
#include <windows.h> // HWND/LRESULT/WPARAM 등 Win32 타입 (WndProcHandler 시그니처용)
#include <string>    // Baker 결과 메시지 / pending load 경로
#include "Core/Math.h" // Matrix (BuildUI view/proj 인자용) — SimpleMath 별칭, 전 구성 가용

// COM 인터페이스 전방 선언 (헤더는 d3d11.h 를 끌어오지 않는다).
struct ID3D11Device;
struct ID3D11DeviceContext;

namespace MiniEngine
{
    class Scene;

    namespace Editor
    {
        // ImGui 기반 에디터 UI. **Editor 구성(WITH_EDITOR)에서만 실제로 동작**하며,
        // 그 외 구성(Debug/Release)에서는 전부 no-op(imgui 심볼 미참조)로 컴파일된다.
        // → Assimp/ImGui는 Editor 전용이라는 §4/§14.2 격리 원칙 준수.
        class EditorUI
        {
        public:
            // ImGui 컨텍스트 + Win32/DX11 백엔드 초기화. 실패 시 false.
            bool Initialize(HWND _hWnd, ID3D11Device* _device, ID3D11DeviceContext* _context);
            void Shutdown();

            // 프레임 시작: ImGui NewFrame + 패널(Hierarchy/Inspector) + 선택 Actor 트랜스폼 기즈모.
            // view/proj 는 ImGuizmo Manipulate 에 전달(선택 없으면 미사용).
            void BuildUI(Scene& _world, const Matrix& _view, const Matrix& _proj);

            // 3D 씬 위에 ImGui draw data를 렌더(현재 바인딩된 RTV 대상).
            void Render();

            // ImGui가 마우스/키보드 입력을 캡처 중인지(게임 입력 게이트용).
            bool WantCaptureMouse() const;
            bool WantCaptureKeyboard() const;

            // 기즈모를 현재 가리키거나 조작 중인지(카메라/피킹 게이트용). Editor 외 항상 false.
            bool IsGizmoActive() const;

            bool IsInitialized() const { return m_initialized; }

            // 선택 Actor 인덱스(World::GetActors 기준). 피킹(GameCore)과 Hierarchy 클릭이 공유.
            void SetSelectedIndex(int _index) { m_selectedIndex = _index; }
            int  GetSelectedIndex() const     { return m_selectedIndex; }

            // Baker 패널의 "Bake & Load" 결과 .mini 경로를 GameCore 가 소비(반환 후 클리어).
            // 대기 중인 경로가 없으면 빈 문자열. (Editor 외 항상 빈 문자열.)
            std::wstring ConsumePendingLoadMini();

        private:
            bool m_initialized = false;
            int  m_selectedIndex = -1; // Hierarchy 선택 인덱스(-1=없음)
            // ImGuizmo 오퍼레이션/모드. ImGuizmo 타입을 헤더에 노출하지 않기 위해 int 로 보관
            // (비-Editor 컴파일 안전). 실제 enum 값은 Initialize()에서 대입.
            int  m_gizmoOp   = 0;
            int  m_gizmoMode = 0;

            // ── Baker 패널 상태(구성 중립) ──
            // 기본값: exe 작업 디렉터리 기준 상대 경로(테스트 애셋 pyramid.obj). 필요 시 UI에서 편집.
            char m_bakerSrc[512]   = "Assets\\pyramid.obj"; // 원본 모델 경로 입력 버퍼
            char m_bakerOut[512]   = "Baked\\pyramid.mini"; // 출력 .mini 경로 입력 버퍼
            char m_bakerAnims[512] = "";                    // 추가 애니 소스(; 구분, 동일 스켈레톤)

            // ── Inspector Animation 섹션 상태(구성 중립) ──
            float m_animFadeSec = 0.3f; // 클립 전환 크로스페이드 시간(초)
            std::wstring m_pendingLoadMini;  // "Bake & Load" 성공 시 씬 로드 대기 경로
            std::string  m_lastBakeMsg;      // 마지막 베이크 결과 메시지
            bool m_lastBakeOk      = false;
            bool m_lastBakeSkinned = false;  // 자동 감지 결과(Skinned/Static)
            int  m_lastBakeVerts   = 0;
            int  m_lastBakeIdx     = 0;
            int  m_lastBakeBones   = 0;      // skinned 일 때만 의미
            int  m_lastBakeClips   = 0;      // skinned 일 때만 의미
        };

        // WndProc 포워딩(main.cpp에서 호출). ImGui가 메시지를 소비하면 non-zero 반환.
        // Editor 외 구성에서는 항상 0(미처리)을 반환한다.
        LRESULT WndProcHandler(HWND _hWnd, UINT _msg, WPARAM _wParam, LPARAM _lParam);
    }
}
