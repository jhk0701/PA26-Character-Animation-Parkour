#include "pch.h"
#include "Platform/Input.h"

namespace MiniEngine
{
    void Input::Initialize(HWND _hWnd)
    {
        m_keyboard = std::make_unique<DirectX::Keyboard>();
        m_mouse = std::make_unique<DirectX::Mouse>();
        m_mouse->SetWindow(_hWnd);

        m_keyTracker.Reset();
        m_mouseTracker.Reset();
    }

    void Input::Update()
    {
        m_keyState = DirectX::Keyboard::Get().GetState();
        m_keyTracker.Update(m_keyState);

        m_mouseState = DirectX::Mouse::Get().GetState();
        m_mouseTracker.Update(m_mouseState);
    }
}
