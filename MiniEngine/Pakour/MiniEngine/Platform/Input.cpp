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

    void Input::Update()
    {
        m_keyState = DirectX::Keyboard::Get().GetState();
        m_keyTracker.Update(m_keyState);

        m_mouseState = DirectX::Mouse::Get().GetState();
        m_mouseTracker.Update(m_mouseState);

        for (auto it = m_mapKeyboardBind.begin(); it != m_mapKeyboardBind.end(); ++it)
        {
            if (it->second.bIsPressed == false && m_keyTracker.IsKeyPressed(it->first))
            {
                it->second.bIsPressed = true;
                it->second.OnPressed();
            }
            else if(it->second.bIsPressed && m_keyTracker.IsKeyPressed(it->first)) 
            {
                // Pressing 필요하면 추가
                // 자리만 마련
            }
            else if (it->second.bIsPressed && m_keyTracker.IsKeyReleased(it->first))
            {
                // Release
                it->second.bIsPressed = false;
                it->second.OnReleased();
            }
        }
    }