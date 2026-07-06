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

    void Input::Clear()
    {
        m_mapKeyboardBind.clear();
    }

    void Input::Update(float _dt)
    {
        m_keyState = DirectX::Keyboard::Get().GetState();
        m_keyTracker.Update(m_keyState);

        m_mouseState = DirectX::Mouse::Get().GetState();
        m_mouseTracker.Update(m_mouseState);

        for (auto it = m_mapKeyboardBind.begin(); it != m_mapKeyboardBind.end(); ++it)
        {
            if (it->second.bIsPressed == false && m_keyTracker.IsKeyPressed(it->first))
            {
                // Pressed 키를 눌렀을 때 1회
                it->second.bIsPressed = true;

                if (it->second.OnPressed)
                    it->second.OnPressed();
            }
            else if (it->second.bIsPressed && m_keyState.IsKeyDown(it->first))
            {
                // Pressing 누르고 있다면 매 프레임마다
                if (it->second.Pressing)
                    it->second.Pressing(_dt);
            }
            else if (it->second.bIsPressed && m_keyTracker.IsKeyReleased(it->first))
            {
                // Released 키에서 손을 뗏을 때 1회
                it->second.bIsPressed = false;

                if (it->second.OnReleased)
                    it->second.OnReleased();
            }
        }
    }
}