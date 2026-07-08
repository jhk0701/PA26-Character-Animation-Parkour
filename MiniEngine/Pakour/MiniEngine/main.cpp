// MiniEngine 진입점. Template 골격(DirectXBase/GameCore) 기반.
#include "pch.h"
#include "GameCore.h"
#include "Editor/EditorUI.h"
#include "Core/Log.h"
#include <directxtk/Keyboard.h>
#include <directxtk/Mouse.h>

namespace
{
    HINSTANCE g_hInst = nullptr;
    HWND      g_hWnd = nullptr;
}

LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);

int APIENTRY wWinMain(_In_ HINSTANCE hInstance,
                      _In_opt_ HINSTANCE hPrevInstance,
                      _In_ LPWSTR lpCmdLine,
                      _In_ int nCmdShow)
{
    UNREFERENCED_PARAMETER(hPrevInstance);
    UNREFERENCED_PARAMETER(lpCmdLine);

    g_hInst = hInstance;

    MiniEngine::Log::Init();
    MG_LOG_INFO("MiniEngine starting");

    const wchar_t* windowClassName = L"MiniEngineWindowClass";
    const wchar_t* windowTitle = L"MiniEngine";

    // 창 클래스 등록.
    WNDCLASSEXW wcex = {};
    wcex.cbSize = sizeof(WNDCLASSEXW);
    wcex.style = CS_HREDRAW | CS_VREDRAW;
    wcex.lpfnWndProc = WndProc;
    wcex.hInstance = hInstance;
    wcex.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wcex.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wcex.lpszClassName = windowClassName;
    RegisterClassExW(&wcex);

    // 창 생성 (배치/크기는 DirectXBase::Init → WindowInit 에서 보정).
    g_hWnd = CreateWindowW(windowClassName, windowTitle, WINDOW_STYLE,
        CW_USEDEFAULT, 0, CW_USEDEFAULT, 0, nullptr, nullptr, hInstance, nullptr);
    if (!g_hWnd)
        return FALSE;

    ShowWindow(g_hWnd, nCmdShow);
    UpdateWindow(g_hWnd);

    std::unique_ptr<GameCore> pCore = std::make_unique<GameCore>();
#ifdef _DEBUG
    assert(pCore->Init(g_hWnd, WINDOW_WIDTH, WINDOW_HEIGHT));
#else
    pCore->Init(g_hWnd, WINDOW_WIDTH, WINDOW_HEIGHT);
#endif

    pCore->BeginPlay();

    // 기본 메시지 루프.
    MSG msg = {};
    while (true)
    {
        if (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE))
        {
            if (msg.message == WM_QUIT)
                break;
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
        else
        {
            pCore->MainLoop();
        }
    }

    pCore->EndPlay();

    return (int)msg.wParam;
}

LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    // ImGui 패널 위 입력 시 return
    if (MiniEngine::Editor::WndProcHandler(hWnd, message, wParam, lParam))
        return true;

    switch (message)
    {
    case WM_ACTIVATE:
    case WM_ACTIVATEAPP:
        DirectX::Keyboard::ProcessMessage(message, wParam, lParam);
        DirectX::Mouse::ProcessMessage(message, wParam, lParam);
        break;

    case WM_INPUT:
    case WM_MOUSEMOVE:
    case WM_LBUTTONDOWN:
    case WM_LBUTTONUP:
    case WM_RBUTTONDOWN:
    case WM_RBUTTONUP:
    case WM_MBUTTONDOWN:
    case WM_MBUTTONUP:
    case WM_MOUSEWHEEL:
    case WM_XBUTTONDOWN:
    case WM_XBUTTONUP:
    case WM_MOUSEHOVER:
        DirectX::Mouse::ProcessMessage(message, wParam, lParam);
        break;

    case WM_KEYDOWN:
    case WM_KEYUP:
        DirectX::Keyboard::ProcessMessage(message, wParam, lParam);
        break;

    case WM_SYSKEYDOWN:
    case WM_SYSKEYUP:
        // 시스템 키(Alt 등)는 포워딩 후에도 DefWindowProc에 넘겨 Alt+F4/시스템 메뉴 유지.
        DirectX::Keyboard::ProcessMessage(message, wParam, lParam);
        return DefWindowProc(hWnd, message, wParam, lParam);

    case WM_DESTROY:
        PostQuitMessage(0);
        break;
    default:
        return DefWindowProc(hWnd, message, wParam, lParam);
    }
    return 0;
}
