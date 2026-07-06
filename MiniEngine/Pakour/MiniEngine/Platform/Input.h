#pragma once
#include <windows.h>
#include <memory>
#include <directxtk/Keyboard.h>
#include <directxtk/Mouse.h>

#include <map>
#include <functional>

// DirectXTK Keyboard/Mouse(MIT) 얇은 래퍼.
// - WndProc는 DirectX::Keyboard/Mouse::ProcessMessage 로 메시지를 직접 포워딩(정적 함수).
// - Input 인스턴스는 매 프레임 Update()로 상태 트래커를 갱신하고 질의 API를 제공.
namespace MiniEngine
{
    struct KeyBind
    {
        bool bIsPressed;
        std::function<void()> OnPressed;
        std::function<void()> OnReleased;
    };

    class Input
    {
    public:
        // 프로젝트 어디서나 쓰기 쉽도록 DirectXTK 키 열거형을 그대로 노출.
        using Keys = DirectX::Keyboard::Keys;

        // Keyboard/Mouse 싱글턴 생성 + 마우스에 창 연결.
        // 주의: 창이 표시되기(메시지 수신) 전에 호출해야 ProcessMessage 예외를 피한다.
        void Initialize(HWND _hWnd);
        void Clear();

        // 매 프레임 1회: 현재 상태 스냅샷 + 엣지 트래커 갱신.
        void Update();

        // ----- 키보드 -----
        bool IsKeyDown(Keys _key) const     { return m_keyState.IsKeyDown(_key); }
        bool IsKeyPressed(Keys _key) const  { return m_keyTracker.IsKeyPressed(_key); }
        bool IsKeyReleased(Keys _key) const { return m_keyTracker.IsKeyReleased(_key); }

        // ----- 마우스 -----
        int  MouseX() const       { return m_mouseState.x; }
        int  MouseY() const       { return m_mouseState.y; }
        int  ScrollValue() const  { return m_mouseState.scrollWheelValue; }

        bool LeftDown() const     { return m_mouseState.leftButton; }
        bool RightDown() const    { return m_mouseState.rightButton; }
        bool MiddleDown() const   { return m_mouseState.middleButton; }

        bool LeftPressed() const  { return m_mouseTracker.leftButton  == DirectX::Mouse::ButtonStateTracker::PRESSED; }
        bool RightPressed() const { return m_mouseTracker.rightButton == DirectX::Mouse::ButtonStateTracker::PRESSED; }

        KeyBind& GetKeyBind(Keys _key) { return m_mapKeyboardBind[_key]; }
        
    private:
        std::unique_ptr<DirectX::Keyboard> m_keyboard;
        std::unique_ptr<DirectX::Mouse>    m_mouse;

        DirectX::Keyboard::State                m_keyState;
        DirectX::Keyboard::KeyboardStateTracker m_keyTracker;
        DirectX::Mouse::State                   m_mouseState;
        DirectX::Mouse::ButtonStateTracker      m_mouseTracker;

        std::map<Keys, KeyBind> m_mapKeyboardBind;
    };
}
