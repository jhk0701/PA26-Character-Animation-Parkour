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
    // windows.h min/max 매크로 충돌 회피(NOMINMAX 미정의) — 명시 비교 헬퍼
    constexpr float Minf(float _a, float _b) { return _a < _b ? _a : _b; }
    constexpr float Maxf(float _a, float _b) { return _a > _b ? _a : _b; }

    // 레이 vs 축정렬 바운딩박스(slab 테스트). 교차 시 최근접 진입 t(음수면 원점이 내부)를 outT에 담고 true.
    // origin/dir/min/max 는 동일 좌표계(호출부는 메시 로컬 공간 사용).
    bool RayIntersectsAABB(const Vector3& _origin, const Vector3& _dir,
                           const Vector3& _min, const Vector3& _max, float& _outT)
    {
        float tMin = -FLT_MAX;
        float tMax =  FLT_MAX;
        const float o[3] = { _origin.x, _origin.y, _origin.z };
        const float d[3] = { _dir.x,    _dir.y,    _dir.z    };
        const float lo[3] = { _min.x, _min.y, _min.z };
        const float hi[3] = { _max.x, _max.y, _max.z };

        for (int i = 0; i < 3; ++i)
        {
            if (std::abs(d[i]) < 1e-8f)
            {
                // 이 축으로 평행 — 슬래브 밖이면 교차 없음.
                if (o[i] < lo[i] || o[i] > hi[i])
                    return false;
            }
            else
            {
                float inv = 1.0f / d[i];
                float t1 = (lo[i] - o[i]) * inv;
                float t2 = (hi[i] - o[i]) * inv;
                if (t1 > t2) { const float tmp = t1; t1 = t2; t2 = tmp; }
                tMin = Maxf(tMin, t1);
                tMax = Minf(tMax, t2);
                if (tMin > tMax)
                    return false;
            }
        }
        // 박스가 레이 뒤쪽이면 제외.
        if (tMax < 0.0f)
            return false;
        _outT = tMin; // 원점이 박스 내부면 음수 — 호출부에서 Maxf(0,·) 취급.
        return true;
    }

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
    
    pScnMgr->FixedUpdate(_dt); // 물리연산 처리

    InputManager::GetInstance()->Update(_dt); // 입력 처리

#if defined(WITH_EDITOR)
    // 게임 입력 게이트: ImGui 패널 위 or 기즈모 조작/호버 중이면 카메라·피킹 차단.
    const bool uiGate = m_editor.WantCaptureMouse() || m_editor.IsGizmoActive();
#endif
    
    pScnMgr->Update(_dt); // Actor/컴포넌트 Tick 전파.
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
    // ImGui NewFrame + 패널(Hierarchy/Inspector) + 기즈모. Render()에서 draw data를 실제로 그린다.
    // 기즈모용 view/proj 전달(카메라 없으면 기본 생성자 = identity — Matrix::Identity 정적상수 LNK2001 회피).
    // (Editor 외 구성은 no-op)
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

    // Baker "Bake & Load" 요청 소비 → 베이크된 .mini 를 씬에 스폰(일반화된 Render 로 함께 렌더).
    const std::wstring pending = m_editor.ConsumePendingLoadMini();
    if (!pending.empty())
        SpawnMeshFromMini(pending);
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

bool GameCore::SpawnMeshFromMini(const std::wstring& _miniPath)
{
    // 헤더(16B)만 피크해 assetType 판별 → Static/Skinned 스폰 분기.
    MiniEngine::MiniHeader header = {};
    {
        std::ifstream probe(_miniPath, std::ios::binary);
        probe.read(reinterpret_cast<char*>(&header), sizeof(header));
        if (!probe || header.magic != MiniEngine::MINI_MAGIC)
        {
            MG_LOG_ERROR("GameCore: baked file is not a valid .mini");
            return false;
        }
    }

    // 파일 stem(확장자/경로 제거)을 Actor 이름으로 사용.
    std::wstring stem = _miniPath;
    const size_t slash = stem.find_last_of(L"\\/");
    if (slash != std::wstring::npos) stem = stem.substr(slash + 1);
    const size_t dot = stem.find_last_of(L'.');
    if (dot != std::wstring::npos) stem = stem.substr(0, dot);
    std::string name; // ASCII 파일명 가정(표시용) — 명시 캐스트로 narrowing 경고 회피
    name.reserve(stem.size());
    for (wchar_t wc : stem) name.push_back(static_cast<char>(wc));
    if (name.empty()) name = "BakedMesh";

    if (header.assetType == static_cast<uint32_t>(MiniEngine::MiniAssetType::SkinnedMesh))
    {
        auto mesh = AssetManager::GetInstance()->LoadSkinnedMesh(_miniPath);
        if (!mesh)
        {
            MG_LOG_ERROR("GameCore: failed to load baked skinned .mini into scene");
            return false;
        }

        std::shared_ptr<Scene> pScene = SceneManager::GetInstance()->GetCurrentScene().lock();
        auto actor = pScene->SpawnActor<Actor>();
        actor->SetName(name);
        auto meshComp = actor->AddComponent<SkeletalMeshComponent>();
        meshComp->SetMesh(mesh);
        /*if (!mesh->GetClips().empty())
            meshComp->SetActiveClip(0)*/;

        // 자동 스케일: 원본 단위(예: Mixamo cm ~180유닛)가 카메라(거리 6) 밖일 수 있으므로
        // 정점 AABB 최대 치수가 ~4 유닛이 되도록 균등 스케일. 큐브와 겹치지 않게 -X 오프셋.
        Vector3 aabbMin(FLT_MAX, FLT_MAX, FLT_MAX);
        Vector3 aabbMax(-FLT_MAX, -FLT_MAX, -FLT_MAX);
        for (const auto& v : mesh->GetVertices())
        {
            aabbMin.x = Minf(aabbMin.x, v.position[0]); aabbMin.y = Minf(aabbMin.y, v.position[1]); aabbMin.z = Minf(aabbMin.z, v.position[2]);
            aabbMax.x = Maxf(aabbMax.x, v.position[0]); aabbMax.y = Maxf(aabbMax.y, v.position[1]); aabbMax.z = Maxf(aabbMax.z, v.position[2]);
        }
        const Vector3 size = aabbMax - aabbMin;
        const float maxDim = Maxf(size.x, Maxf(size.y, size.z));
        float scale = 1.0f;
        if (maxDim > 1e-4f)
            scale = 4.0f / maxDim;
        meshComp->localTransform.scale = Vector3(scale, scale, scale);
        meshComp->localTransform.position = Vector3(-3.0f, -2.0f, 0.0f);
        MG_LOG_INFO("GameCore: spawned baked skinned actor ({} bones, {} clips, maxDim {:.1f}, scale {:.4f})",
            mesh->GetSkeleton().bones.size(), mesh->GetClips().size(), maxDim, scale);
        return true;
    }

    auto mesh = AssetManager::GetInstance()->LoadStaticMesh(_miniPath);
    if (!mesh)
    {
        MG_LOG_ERROR("GameCore: failed to load baked .mini into scene");
        return false;
    }

    std::shared_ptr<Scene> pScene = SceneManager::GetInstance()->GetCurrentScene().lock();
    auto actor = pScene->SpawnActor<Actor>();
    actor->SetName(name);
    auto meshComp = actor->AddComponent<StaticMeshComponent>();
    meshComp->SetMesh(mesh);
    MG_LOG_INFO("GameCore: spawned baked mesh actor into scene");
    return true;
}