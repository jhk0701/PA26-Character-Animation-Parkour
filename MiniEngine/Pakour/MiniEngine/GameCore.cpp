#include "pch.h"
#include "GameCore.h"
#include "Core/Log.h"
#include "Scene/Actor.h"
#include "Scene/SceneComponent.h"
#include "Asset/MiniLoader.h"
#include <fstream>
#include <cfloat>
#include <cmath>

using namespace MiniEngine;

namespace
{
    // windows.h min/max 매크로 충돌 회피(NOMINMAX 미정의) — 명시 비교 헬퍼. (§증분6 관례)
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

    // b0: per-object. row_major 무전치 업로드(셰이더 cbuffer 규약과 일치).
    struct PerObjectCB
    {
        Matrix mvp;
        Matrix world;
    };

    // b1: per-frame 라이트/머티리얼. HLSL cbuffer 레이아웃(16바이트 정렬)과 일치.
    struct PerFrameCB
    {
        Vector3 lightDir;   float ambient;
        Vector3 lightColor; float pad0;
        Vector3 albedo;     float pad1;
    };

    // b2: 본 최종 행렬. SkinnedMeshLambert.hlsl 의 MAX_BONES 와 일치해야 한다.
    constexpr int MAX_BONES = 128;
    struct BoneCB
    {
        Matrix boneMatrices[MAX_BONES];
    };

    // 셰이더 파일을 런타임 컴파일. 실패 시 에러 메시지를 디버그 출력.
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

    // 실행 파일 위치 기준 디렉터리(작업 디렉터리 무관).
    std::wstring ExeDir()
    {
        wchar_t exePath[MAX_PATH] = {};
        GetModuleFileNameW(nullptr, exePath, MAX_PATH);
        std::wstring dir(exePath);
        const size_t slash = dir.find_last_of(L"\\/");
        if (slash != std::wstring::npos)
            dir.resize(slash);
        return dir;
    }

    // exe 기준 Shaders\<name> 절대 경로.
    std::wstring ResolveShaderPath(const wchar_t* _fileName)
    {
        return ExeDir() + L"\\Shaders\\" + _fileName;
    }

    // exe 기준 Assets\<name> 절대 경로.
    std::wstring ResolveAssetPath(const wchar_t* _fileName)
    {
        return ExeDir() + L"\\Assets\\" + _fileName;
    }
}

GameCore::GameCore()
{
}

GameCore::~GameCore()
{
    // 디바이스가 살아있는 동안 ImGui 백엔드를 먼저 정리. (Editor 외 구성은 no-op)
    m_editor.Shutdown();
}

bool GameCore::Init(HWND _hWnd, int _iWidth, int _iHeight)
{
    // 창이 표시(WindowInit)되어 메시지를 받기 전에 Keyboard/Mouse 싱글턴을 먼저 생성해야
    // WndProc의 ProcessMessage가 예외 없이 동작한다.
    m_input.Initialize(_hWnd);
    InitDefaultInput();

    if (DirectXBase::Init(_hWnd, _iWidth, _iHeight) == false)
        return false;

    if (InitRenderResources() == false)
        return false;
    if (InitMeshScene() == false)
        return false;
    if (InitSkinnedScene() == false)
        return false;
    

    // 카메라 Actor.
    m_cameraActor = m_world.SpawnActor<Actor>();
    m_cameraActor->SetName("Camera");
    auto camera = m_cameraActor->AddComponent<CameraComponent>();
    camera->aspect = static_cast<float>(_iWidth) / static_cast<float>(_iHeight);
    m_camera = camera; // 비소유 캐시
    m_camController.Initialize(Vector3(0.0f, 0.0f, 0.0f), 6.0f);

    m_world.Construct();
    m_world.BeginPlay();

    // 에디터 UI 초기화 (Editor 구성에서만 실제 동작).
    m_editor.Initialize(_hWnd, m_device.Get(), m_context.Get());

    MG_LOG_INFO("GameCore initialized ({}x{}) - static mesh (.mini) + Lambert", _iWidth, _iHeight);
    return true;
}

void GameCore::InitDefaultInput()
{
    m_input.GetKeyBind(DirectX::Keyboard::Keys::Escape).OnPressed = std::bind([this]() { QuitGame(); });
}


bool GameCore::InitRenderResources()
{
    const std::wstring shaderPath = ResolveShaderPath(L"StaticMeshLambert.hlsl");

    // Vertex Shader.
    Microsoft::WRL::ComPtr<ID3DBlob> vsBlob;
    HRESULT hr = CompileShaderFromFile(shaderPath, "VSMain", "vs_5_0", &vsBlob);
    assert(SUCCEEDED(hr));
    if (FAILED(hr)) return false;

    hr = m_device->CreateVertexShader(vsBlob->GetBufferPointer(), vsBlob->GetBufferSize(), nullptr, &m_vertexShader);
    assert(SUCCEEDED(hr));
    if (FAILED(hr)) return false;

    // 입력 레이아웃: POSITION(0) / NORMAL(12) / TEXCOORD(24) — MiniStaticVertex 레이아웃과 일치.
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

    // ---- GPU 스키닝 리소스 (SkinnedMeshLambert.hlsl) ----
    const std::wstring skinnedPath = ResolveShaderPath(L"SkinnedMeshLambert.hlsl");

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

bool GameCore::InitMeshScene()
{
    // exe 옆 Assets\ 폴더에 큐브 .mini 를 (없으면) 절차적으로 생성 후 로드.
    const std::wstring assetPath = ResolveAssetPath(L"Cube.mini");
    CreateDirectoryW((ExeDir() + L"\\Assets").c_str(), nullptr); // 이미 있으면 무시됨

    std::ifstream probe(assetPath, std::ios::binary);
    const bool exists = probe.is_open();
    probe.close();
    if (!exists)
    {
        if (!MiniLoader::WriteCubeMini(assetPath))
        {
            MG_LOG_ERROR("GameCore: failed to write Cube.mini");
            return false;
        }
    }

    auto mesh = m_assets.LoadStaticMesh(assetPath, m_device.Get());
    if (!mesh)
    {
        MG_LOG_ERROR("GameCore: failed to load Cube.mini");
        return false;
    }

    // 메시 Actor 스폰 (루트를 StaticMeshComponent 로). 소유는 World, GameCore는 비소유 캐시.
    auto actor = m_world.SpawnActor<Actor>();
    actor->SetName("Cube");
    auto meshComp = actor->AddComponent<StaticMeshComponent>();
    meshComp->SetMesh(mesh);
    m_meshActor = actor;
    m_meshComponent = meshComp;

    return true;
}

bool GameCore::InitSkinnedScene()
{
    // exe 옆 Assets\ 폴더에 스키닝 테스트 박스 .mini 를 (없으면) 절차적으로 생성 후 로드.
    const std::wstring assetPath = ResolveAssetPath(L"SkinnedTest.mini");
    CreateDirectoryW((ExeDir() + L"\\Assets").c_str(), nullptr); // 이미 있으면 무시됨

    std::ifstream probe(assetPath, std::ios::binary);
    const bool exists = probe.is_open();
    probe.close();
    if (!exists)
    {
        if (!MiniLoader::WriteSkinnedTest(assetPath))
        {
            MG_LOG_ERROR("GameCore: failed to write SkinnedTest.mini");
            return false;
        }
    }

    auto mesh = m_assets.LoadSkinnedMesh(assetPath, m_device.Get());
    if (!mesh)
    {
        MG_LOG_ERROR("GameCore: failed to load SkinnedTest.mini");
        return false;
    }

    // 스키닝 Actor 스폰(큐브 옆 +X 오프셋). 소유는 World.
    auto actor = m_world.SpawnActor<Actor>();
    actor->SetName("SkinnedTest");
    auto meshComp = actor->AddComponent<SkeletalMeshComponent>();
    meshComp->SetMesh(mesh);
    meshComp->localTransform.position = Vector3(3.0f, -1.5f, 0.0f);
    meshComp->SetActiveClip(0); // "wave" 재생
    return true;
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
        auto mesh = m_assets.LoadSkinnedMesh(_miniPath, m_device.Get());
        if (!mesh)
        {
            MG_LOG_ERROR("GameCore: failed to load baked skinned .mini into scene");
            return false;
        }

        auto actor = m_world.SpawnActor<Actor>();
        actor->SetName(name);
        auto meshComp = actor->AddComponent<SkeletalMeshComponent>();
        meshComp->SetMesh(mesh);
        if (!mesh->GetClips().empty())
            meshComp->SetActiveClip(0);

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
        meshComp->localTransform.scale    = Vector3(scale, scale, scale);
        meshComp->localTransform.position = Vector3(-3.0f, -2.0f, 0.0f);
        MG_LOG_INFO("GameCore: spawned baked skinned actor ({} bones, {} clips, maxDim {:.1f}, scale {:.4f})",
                    mesh->GetSkeleton().bones.size(), mesh->GetClips().size(), maxDim, scale);
        return true;
    }

    auto mesh = m_assets.LoadStaticMesh(_miniPath, m_device.Get());
    if (!mesh)
    {
        MG_LOG_ERROR("GameCore: failed to load baked .mini into scene");
        return false;
    }

    auto actor = m_world.SpawnActor<Actor>();
    actor->SetName(name);
    auto meshComp = actor->AddComponent<StaticMeshComponent>();
    meshComp->SetMesh(mesh);
    MG_LOG_INFO("GameCore: spawned baked mesh actor into scene");
    return true;
}

void GameCore::Update(float _dt)
{
    m_input.Update();

    // 게임 입력 게이트: ImGui 패널 위 or 기즈모 조작/호버 중이면 카메라·피킹 차단.
    const bool uiGate = m_editor.WantCaptureMouse() || m_editor.IsGizmoActive();

    // 카메라 조작 (입력 → 카메라 트랜스폼).
    if (!uiGate)
    {
        if (auto camera = m_camera.lock())
            m_camController.Update(_dt, m_input, *camera);
    }

    // 좌클릭 피킹. 미스는 -1로 선택 해제.
    if (!uiGate && m_input.LeftPressed())
    {
        if (auto camera = m_camera.lock())
            m_editor.SetSelectedIndex(PickActor(*camera));
    }

    // 메시 자전 데모: Editor 구성(기즈모 조작 대상)은 정지, 그 외(스텁 IsInitialized=false)만 자전.
    if (!m_editor.IsInitialized())
    {
        m_meshAngle += _dt * 0.5f;
        if (auto meshComp = m_meshComponent.lock())
            meshComp->localTransform.rotation =
                Quaternion::CreateFromYawPitchRoll(m_meshAngle, m_meshAngle * 0.6f, 0.0f);
    }

    // Actor/컴포넌트 Tick 전파.
    m_world.Tick(_dt);
}

void GameCore::Render()
{
    constexpr float clearColor[4] = { 0.1f, 0.1f, 0.12f, 1.0f };
    RenderBegin(clearColor);

    // 월드의 모든 StaticMeshComponent / SkeletalMeshComponent 를 그린다(피킹 순회와 동일).
    // 베이크로 스폰된 메시도 포함.
    if (std::shared_ptr<CameraComponent> camera = m_camera.lock())
    {
        for (const auto& actor : m_world.GetActors())
        {
            if (auto meshComp = actor->GetComponent<StaticMeshComponent>())
            {
                auto mesh = meshComp->GetMesh();
                if (mesh && mesh->HasGpuResources())
                    DrawMesh(*camera, *meshComp);
            }

            if (auto skelComp = actor->GetComponent<SkeletalMeshComponent>())
            {
                auto mesh = skelComp->GetMesh();
                if (mesh && mesh->HasGpuResources())
                    DrawSkinnedMesh(*camera, *skelComp);
            }
        }
    }

    // ImGui 오버레이는 씬 위에 항상 그린다(메시가 없어도 UpdateGUI의 NewFrame을 마무리).
    m_editor.Render();
    RenderEnd();
}

// 선택된 카메라로 메시 컴포넌트를 Lambert 셰이딩으로 그린다.
void GameCore::DrawMesh(MiniEngine::CameraComponent& _camera, MiniEngine::StaticMeshComponent& _meshComp)
{
    auto mesh = _meshComp.GetMesh();

    // MVP = world * view * proj (row-vector, 전치 없음 — 셰이더 cbuffer는 row_major).
    const Matrix world = _meshComp.GetWorldMatrix();
    const Matrix view  = _camera.GetViewMatrix();
    const Matrix proj  = _camera.GetProjectionMatrix();

    PerObjectCB perObject = {};
    perObject.mvp   = world * view * proj;
    perObject.world = world;

    D3D11_MAPPED_SUBRESOURCE mapped = {};
    if (SUCCEEDED(m_context->Map(m_perObjectCB.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped)))
    {
        memcpy(mapped.pData, &perObject, sizeof(perObject));
        m_context->Unmap(m_perObjectCB.Get(), 0);
    }

    // per-frame 라이트/알베도. lightDir = 빛이 나아가는 방향(정규화). PS 에서 -l 로 N·L 계산.
    PerFrameCB perFrame = {};
    Vector3 dir(-0.4f, -1.0f, -0.6f);
    dir.Normalize();
    perFrame.lightDir   = dir;
    perFrame.ambient    = 0.15f;
    perFrame.lightColor = Vector3(1.0f, 1.0f, 1.0f);
    perFrame.albedo     = Vector3(0.85f, 0.78f, 0.70f);

    if (SUCCEEDED(m_context->Map(m_perFrameCB.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped)))
    {
        memcpy(mapped.pData, &perFrame, sizeof(perFrame));
        m_context->Unmap(m_perFrameCB.Get(), 0);
    }

    UINT stride = mesh->GetVertexStride();
    UINT offset = 0;
    ID3D11Buffer* vb        = mesh->GetVertexBuffer();
    ID3D11Buffer* objectCB  = m_perObjectCB.Get();
    ID3D11Buffer* frameCB   = m_perFrameCB.Get();

    m_context->IASetInputLayout(m_inputLayout.Get());
    m_context->IASetVertexBuffers(0, 1, &vb, &stride, &offset);
    m_context->IASetIndexBuffer(mesh->GetIndexBuffer(), DXGI_FORMAT_R32_UINT, 0);
    m_context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    m_context->VSSetShader(m_vertexShader.Get(), nullptr, 0);
    m_context->VSSetConstantBuffers(0, 1, &objectCB);
    m_context->PSSetShader(m_pixelShader.Get(), nullptr, 0);
    m_context->PSSetConstantBuffers(1, 1, &frameCB);
    m_context->DrawIndexed(mesh->GetIndexCount(), 0, 0);
}

// 선택된 카메라로 스키닝 메시를 GPU 스키닝 + Lambert 로 그린다.
void GameCore::DrawSkinnedMesh(MiniEngine::CameraComponent& _camera, MiniEngine::SkeletalMeshComponent& _meshComp)
{
    auto mesh = _meshComp.GetMesh();

    // b0/b1 은 DrawMesh 와 동일 규약 (row-vector, 무전치).
    const Matrix world = _meshComp.GetWorldMatrix();
    const Matrix view  = _camera.GetViewMatrix();
    const Matrix proj  = _camera.GetProjectionMatrix();

    PerObjectCB perObject = {};
    perObject.mvp   = world * view * proj;
    perObject.world = world;

    D3D11_MAPPED_SUBRESOURCE mapped = {};
    if (SUCCEEDED(m_context->Map(m_perObjectCB.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped)))
    {
        memcpy(mapped.pData, &perObject, sizeof(perObject));
        m_context->Unmap(m_perObjectCB.Get(), 0);
    }

    PerFrameCB perFrame = {};
    Vector3 dir(-0.4f, -1.0f, -0.6f);
    dir.Normalize();
    perFrame.lightDir   = dir;
    perFrame.ambient    = 0.15f;
    perFrame.lightColor = Vector3(1.0f, 1.0f, 1.0f);
    perFrame.albedo     = Vector3(0.55f, 0.75f, 0.85f); // 큐브와 구분되는 한색 계열

    if (SUCCEEDED(m_context->Map(m_perFrameCB.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped)))
    {
        memcpy(mapped.pData, &perFrame, sizeof(perFrame));
        m_context->Unmap(m_perFrameCB.Get(), 0);
    }

    // b2: 본 최종 행렬 업로드 (부족분은 identity 패딩 — WRITE_DISCARD 라 전체를 채운다).
    const std::vector<Matrix>& bones = _meshComp.GetBoneMatrices();
    if (SUCCEEDED(m_context->Map(m_boneCB.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped)))
    {
        BoneCB* boneCB = static_cast<BoneCB*>(mapped.pData);
        const size_t count = (bones.size() < static_cast<size_t>(MAX_BONES)) ? bones.size() : MAX_BONES;
        for (size_t i = 0; i < count; ++i)
            boneCB->boneMatrices[i] = bones[i];
        for (size_t i = count; i < static_cast<size_t>(MAX_BONES); ++i)
            boneCB->boneMatrices[i] = Matrix(); // identity
        m_context->Unmap(m_boneCB.Get(), 0);
    }

    UINT stride = mesh->GetVertexStride();
    UINT offset = 0;
    ID3D11Buffer* vb       = mesh->GetVertexBuffer();
    ID3D11Buffer* objectCB = m_perObjectCB.Get();
    ID3D11Buffer* frameCB  = m_perFrameCB.Get();
    ID3D11Buffer* boneCB   = m_boneCB.Get();

    m_context->IASetInputLayout(m_skinnedInputLayout.Get());
    m_context->IASetVertexBuffers(0, 1, &vb, &stride, &offset);
    m_context->IASetIndexBuffer(mesh->GetIndexBuffer(), DXGI_FORMAT_R32_UINT, 0);
    m_context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    m_context->VSSetShader(m_skinnedVertexShader.Get(), nullptr, 0);
    m_context->VSSetConstantBuffers(0, 1, &objectCB);
    m_context->VSSetConstantBuffers(2, 1, &boneCB);
    m_context->PSSetShader(m_skinnedPixelShader.Get(), nullptr, 0);
    m_context->PSSetConstantBuffers(1, 1, &frameCB);
    m_context->DrawIndexed(mesh->GetIndexCount(), 0, 0);
}

int GameCore::PickActor(const CameraComponent& _camera) const
{
    if (m_iWindowWidth <= 0 || m_iWindowHeight <= 0)
        return -1;

    // 마우스 픽셀 → NDC(-1..1, y 반전).
    const float ndcX = 2.0f * static_cast<float>(m_input.MouseX()) / static_cast<float>(m_iWindowWidth) - 1.0f;
    const float ndcY = 1.0f - 2.0f * static_cast<float>(m_input.MouseY()) / static_cast<float>(m_iWindowHeight);

    // 뷰·프로젝션 역행렬로 NDC 근/원점을 월드로 언프로젝트(DX NDC z ∈ [0,1]).
    const Matrix invVP = (_camera.GetViewMatrix() * _camera.GetProjectionMatrix()).Invert();
    const Vector3 nearW = Vector3::Transform(Vector3(ndcX, ndcY, 0.0f), invVP);
    const Vector3 farW  = Vector3::Transform(Vector3(ndcX, ndcY, 1.0f), invVP);
    Vector3 rayO = nearW;
    Vector3 rayD = farW - nearW;
    rayD.Normalize();

    int   bestIndex = -1;
    float bestT     = FLT_MAX;

    const auto& actors = m_world.GetActors();
    for (int i = 0; i < static_cast<int>(actors.size()); ++i)
    {
        auto meshComp = actors[i]->GetComponent<StaticMeshComponent>();
        if (!meshComp)
            continue;
        auto mesh = meshComp->GetMesh();
        if (!mesh)
            continue;

        const auto& verts = mesh->GetVertices();
        if (verts.empty())
            continue;

        // 레이를 메시 로컬 공간으로 변환(아핀 선형성으로 로컬 t == 월드 거리).
        const Matrix invW = meshComp->GetWorldMatrix().Invert();
        const Vector3 localO = Vector3::Transform(rayO, invW);
        const Vector3 localD = Vector3::TransformNormal(rayD, invW);

        // 로컬 AABB(정점 min/max).
        Vector3 aabbMin(FLT_MAX, FLT_MAX, FLT_MAX);
        Vector3 aabbMax(-FLT_MAX, -FLT_MAX, -FLT_MAX);
        for (const auto& v : verts)
        {
            aabbMin.x = Minf(aabbMin.x, v.position[0]); aabbMin.y = Minf(aabbMin.y, v.position[1]); aabbMin.z = Minf(aabbMin.z, v.position[2]);
            aabbMax.x = Maxf(aabbMax.x, v.position[0]); aabbMax.y = Maxf(aabbMax.y, v.position[1]); aabbMax.z = Maxf(aabbMax.z, v.position[2]);
        }

        float t;
        if (RayIntersectsAABB(localO, localD, aabbMin, aabbMax, t))
        {
            const float hitT = Maxf(0.0f, t);
            if (hitT < bestT)
            {
                bestT     = hitT;
                bestIndex = i;
            }
        }
    }

    return bestIndex;
}

void GameCore::UpdateGUI()
{
    // ImGui NewFrame + 패널(Hierarchy/Inspector) + 기즈모. Render()에서 draw data를 실제로 그린다.
    // 기즈모용 view/proj 전달(카메라 없으면 기본 생성자 = identity — Matrix::Identity 정적상수 LNK2001 회피).
    // (Editor 외 구성은 no-op)
    Matrix view, proj;
    if (auto camera = m_camera.lock())
    {
        view = camera->GetViewMatrix();
        proj = camera->GetProjectionMatrix();
    }
    m_editor.BuildUI(m_world, view, proj);

    // Baker "Bake & Load" 요청 소비 → 베이크된 .mini 를 씬에 스폰(일반화된 Render 로 함께 렌더).
    const std::wstring pending = m_editor.ConsumePendingLoadMini();
    if (!pending.empty())
        SpawnMeshFromMini(pending);
}

void GameCore::QuitGame()
{
    MG_LOG_INFO("Escape pressed - quitting");
    PostQuitMessage(0);
}
