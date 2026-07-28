#include "pch.h"
#include "GameCore.h"
#include "Core/Log.h"
#include "Manager/PathManager.h"
#include "Manager/AssetManager.h"
#include "Manager/SceneManager.h"
#include "Manager/UIManager.h"
#include "Platform/Input.h" // Input 

#include <fstream>
#include <cfloat>
#include <cmath>
#include <directxtk/SimpleMath.h>
#include <physx/PxPhysicsAPI.h>

// 테스트용 추가
#include "Scene/Actor.h"
#include "Scene/CameraComponent.h"
#include "Scene/Scene.h"
#include "Scene/StaticMeshComponent.h"
#include "Scene/SkeletalMeshComponent.h"

using namespace MiniEngine;
using namespace MiniEngine::Graphics;

namespace
{
    constexpr float Minf(float _a, float _b) { return _a < _b ? _a : _b; }
    constexpr float Maxf(float _a, float _b) { return _a > _b ? _a : _b; }

    // 셰이더 파일을 런타임 컴파일
    // 실패 시 에러 메시지를 디버그 출력
    HRESULT CompileShaderFromFile(const std::wstring& _path, const char* _entry, const char* _target, ID3DBlob** _outBlob)
    {
        UINT flags = 0;
#if defined(_DEBUG) || defined(WITH_EDITOR)
        flags |= D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
#endif
        Microsoft::WRL::ComPtr<ID3DBlob> errorBlob;
        HRESULT hr = D3DCompileFromFile(_path.c_str(), nullptr, nullptr, _entry, _target, flags, 0, _outBlob, &errorBlob);
        if (FAILED(hr) && errorBlob)
            OutputDebugStringA(static_cast<const char*>(errorBlob->GetBufferPointer()));
        return hr;
    }
}

GameCore::GameCore() { }
GameCore::~GameCore()
{
    m_editor.Shutdown();
}

bool GameCore::Init(HWND _hWnd, int _iWidth, int _iHeight)
{
    // 창이 표시(WindowInit)되어 메시지를 받기 전에 Keyboard/Mouse 싱글턴을 먼저 생성 -> WndProc의 ProcessMessage가 예외 없이 동작할 것
    InputManager::GetInstance()->Init(_hWnd);

    if (DirectXBase::Init(_hWnd, _iWidth, _iHeight) == false)
        return false;

    if (InitRenderResources() == false)
        return false;

    PathManager::GetInstance()->Init();
    AssetManager::GetInstance()->Init(m_device.Get());
    SceneManager::GetInstance()->Init(m_device.Get(), m_context.Get());

    // 에디터 UI 초기화 (Editor 구성에서만 실제 동작).
    m_editor.Initialize(_hWnd, m_device.Get(), m_context.Get());

    UIManager::GetInstance()->Init(_hWnd, m_device.Get(), m_context.Get());

    MG_LOG_INFO("GameCore initialized ({}x{}) - static mesh (.mini) + Lambert", _iWidth, _iHeight);

    return true;
}

bool GameCore::InitRenderResources()
{
    PathManager* pathMgr = PathManager::GetInstance();
    const std::wstring shaderPath = pathMgr->ResolveShaderPath(L"StaticMeshLambert.hlsl");

    // Vertex Shader.
    Microsoft::WRL::ComPtr<ID3DBlob> vsBlob;
    HRESULT hr = CompileShaderFromFile(shaderPath, "VSMain", "vs_5_0", &vsBlob);
    assert(SUCCEEDED(hr));
    if (FAILED(hr)) return false;

    hr = m_device->CreateVertexShader(vsBlob->GetBufferPointer(), vsBlob->GetBufferSize(), nullptr, &m_vertexShader);
    assert(SUCCEEDED(hr));
    if (FAILED(hr)) return false;

    // 입력 레이아웃: POSITION(0) / NORMAL(12) / TEXCOORD(24) — MiniStaticVertex 레이아웃
    const D3D11_INPUT_ELEMENT_DESC layout[] =
    {
        { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0,  D3D11_INPUT_PER_VERTEX_DATA, 0 },
        { "NORMAL",   0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0 },
        { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT,    0, 24, D3D11_INPUT_PER_VERTEX_DATA, 0 },
    };
    hr = m_device->CreateInputLayout(layout, ARRAYSIZE(layout), vsBlob->GetBufferPointer(), vsBlob->GetBufferSize(), &m_inputLayout);
    assert(SUCCEEDED(hr));
    if (FAILED(hr)) return false;

    // Pixel Shader.
    Microsoft::WRL::ComPtr<ID3DBlob> psBlob;
    hr = CompileShaderFromFile(shaderPath, "PSMain", "ps_5_0", &psBlob);
    assert(SUCCEEDED(hr));
    if (FAILED(hr)) return false;

    hr = m_device->CreatePixelShader(psBlob->GetBufferPointer(), psBlob->GetBufferSize(), nullptr, &m_pixelShader);
    assert(SUCCEEDED(hr));
    if (FAILED(hr)) return false;

    // per-object 상수버퍼 (DYNAMIC, 매 프레임 갱신). mvp + world = 128바이트.
    D3D11_BUFFER_DESC cbDesc = {};
    cbDesc.Usage          = D3D11_USAGE_DYNAMIC;
    cbDesc.ByteWidth      = sizeof(PerObjectCB);
    cbDesc.BindFlags      = D3D11_BIND_CONSTANT_BUFFER;
    cbDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
    hr = m_device->CreateBuffer(&cbDesc, nullptr, &m_perObjectCB);
    assert(SUCCEEDED(hr));
    if (FAILED(hr)) return false;

    // per-frame 상수버퍼 (DYNAMIC). 라이트/알베도 = 48바이트(16 정렬).
    cbDesc.ByteWidth = sizeof(PerFrameCB);
    hr = m_device->CreateBuffer(&cbDesc, nullptr, &m_perFrameCB);
    assert(SUCCEEDED(hr));
    if (FAILED(hr)) return false;

    // GPU 스키닝 리소스 (SkinnedMeshLambert.hlsl)
    const std::wstring skinnedPath = pathMgr->ResolveShaderPath(L"SkinnedMeshLambert.hlsl");

    Microsoft::WRL::ComPtr<ID3DBlob> skinnedVsBlob;
    hr = CompileShaderFromFile(skinnedPath, "VSMain", "vs_5_0", &skinnedVsBlob);
    assert(SUCCEEDED(hr));
    if (FAILED(hr)) return false;

    hr = m_device->CreateVertexShader(skinnedVsBlob->GetBufferPointer(), skinnedVsBlob->GetBufferSize(), nullptr, &m_skinnedVertexShader);
    assert(SUCCEEDED(hr));
    if (FAILED(hr)) return false;

    // 스키닝 입력 레이아웃 — MiniSkinnedVertex(64B) 와 일치.
    const D3D11_INPUT_ELEMENT_DESC skinnedLayout[] =
    {
        { "POSITION",    0, DXGI_FORMAT_R32G32B32_FLOAT,    0, 0,  D3D11_INPUT_PER_VERTEX_DATA, 0 },
        { "NORMAL",      0, DXGI_FORMAT_R32G32B32_FLOAT,    0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0 },
        { "TEXCOORD",    0, DXGI_FORMAT_R32G32_FLOAT,       0, 24, D3D11_INPUT_PER_VERTEX_DATA, 0 },
        { "BONEINDICES", 0, DXGI_FORMAT_R32G32B32A32_UINT,  0, 32, D3D11_INPUT_PER_VERTEX_DATA, 0 },
        { "BONEWEIGHTS", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 48, D3D11_INPUT_PER_VERTEX_DATA, 0 },
    };
    hr = m_device->CreateInputLayout(skinnedLayout, ARRAYSIZE(skinnedLayout),
                                     skinnedVsBlob->GetBufferPointer(), skinnedVsBlob->GetBufferSize(), &m_skinnedInputLayout);
    assert(SUCCEEDED(hr));
    if (FAILED(hr)) return false;

    Microsoft::WRL::ComPtr<ID3DBlob> skinnedPsBlob;
    hr = CompileShaderFromFile(skinnedPath, "PSMain", "ps_5_0", &skinnedPsBlob);
    assert(SUCCEEDED(hr));
    if (FAILED(hr)) return false;

    hr = m_device->CreatePixelShader(skinnedPsBlob->GetBufferPointer(), skinnedPsBlob->GetBufferSize(), nullptr, &m_skinnedPixelShader);
    assert(SUCCEEDED(hr));
    if (FAILED(hr)) return false;

    // 본 행렬 상수버퍼 (DYNAMIC, 매 드로우 갱신). 128 * 64 = 8192바이트.
    cbDesc.ByteWidth = sizeof(BoneCB);
    hr = m_device->CreateBuffer(&cbDesc, nullptr, &m_boneCB);
    assert(SUCCEEDED(hr));
    if (FAILED(hr)) return false;

    return true;
}

void GameCore::BeginPlay()
{
    // BeginPlay 전파
    SceneManager::GetInstance()->BeginPlay();
}

void GameCore::Update(float _dt)
{
    SceneManager* pScnMgr = SceneManager::GetInstance();
    
    pScnMgr->FixedUpdate(_dt); // 물리연산 처리용

    InputManager::GetInstance()->Update(_dt); // 입력 처리

#if defined(WITH_EDITOR)
    // 게임 입력 게이트: ImGui 패널 위 or 기즈모 조작/호버 중이면 카메라·피킹 차단.
    const bool uiGate = m_editor.WantCaptureMouse() || m_editor.IsGizmoActive();
#endif
    
    // Actor/컴포넌트 Tick 전파.
    pScnMgr->Update(_dt);
    pScnMgr->LateUpdate(_dt);
}

void GameCore::Render()
{
    constexpr float clearColor[4] = { 0.1f, 0.1f, 0.1f, 1.0f };
    RenderBegin(clearColor);

    Graphics::RenderContext context;
    context.m_context                   = m_context.Get();
    context.m_perObjectCB               = m_perObjectCB.Get();
    context.m_perFrameCB                = m_perFrameCB.Get();
    context.m_staticMeshInputLayout     = m_inputLayout.Get();
    context.m_staticMeshVS              = m_vertexShader.Get();
    context.m_staticMeshPS              = m_pixelShader.Get();
    context.m_skinnedMeshInputLayout    = m_skinnedInputLayout.Get();
    context.m_skinnedMeshVS             = m_skinnedVertexShader.Get();
    context.m_skinnedMeshPS             = m_skinnedPixelShader.Get();
    context.m_boneCB                    = m_boneCB.Get();
    
    SceneManager::GetInstance()->Render(context); // Actor, 컴포넌트 Render 전파

    // ImGui 오버레이는 씬 위에 항상 그린다(메시가 없어도 UpdateGUI의 NewFrame을 마무리).
    m_editor.Render();
    UIManager::GetInstance()->Render();

    RenderEnd();
}

void GameCore::UpdateGUI()
{
    std::weak_ptr<Scene> pScene = SceneManager::GetInstance()->GetCurrentScene();
    if (pScene.expired())
        return;

    Matrix view, proj;
    if (auto camera = pScene.lock()->GetMainCamera().lock())
    {
        view = camera->GetViewMatrix();
        proj = camera->GetProjectionMatrix();
    }

    m_editor.BuildUI(*pScene.lock(), view, proj);

    UIManager::GetInstance()->BuildUI(*pScene.lock(), view, proj);
}

void GameCore::EndPlay()
{
    // Actor에 EndPlay로 정리
    SceneManager::GetInstance()->EndPlay();
    InputManager::GetInstance()->Clear();
    UIManager::GetInstance()->Shutdown();
}

void GameCore::QuitGame()
{
    MG_LOG_INFO("Escape pressed - quitting");
    PostQuitMessage(0);
}
