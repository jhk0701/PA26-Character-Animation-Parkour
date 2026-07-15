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

        // 첫 실행 시, 마우스 델타가 크게 튀어
        // 초기화시 현재 마우스 위치 가져오기
        m_mouseState = DirectX::Mouse::Get().GetState();
        m_mouseTracker.Update(m_mouseState);
        m_prevMouseX = MouseX();
        m_prevMouseY = MouseY();
    }

    void Input::Clear()
    {
        m_mapKeyboardBind.clear();
    }

    void Input::Update(float _dt)
    {
        // 마우스 입력 업데이트
        m_mouseState = DirectX::Mouse::Get().GetState();
        m_mouseTracker.Update(m_mouseState);

        m_mouseDelta.x = static_cast<float>(MouseX() - m_prevMouseX);
        m_mouseDelta.y = static_cast<float>(m_prevMouseY - MouseY());
        m_mouseDelta *= _dt;

        m_prevMouseX = MouseX();
        m_prevMouseY = MouseY();

        // 키보드 입력 업데이트
        m_keyState = DirectX::Keyboard::Get().GetState();
        m_keyTracker.Update(m_keyState);

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

    InputManager::InputManager() {};
    InputManager::~InputManager() {};

    void InputManager::Init(HWND _hWnd) 
    {
        m_input.Initialize(_hWnd);
    }
    void InputManager::Clear()
    {
        m_input.Clear();
    }
    void InputManager::Update(float _dt)
    {
        m_input.Update(_dt);
    }
}