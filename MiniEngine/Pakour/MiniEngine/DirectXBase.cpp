#include "pch.h"
#include "DirectXBase.h"


DirectXBase::DirectXBase()
{
	m_iWindowHeight = 0;
	m_iWindowWidth = 0;
	m_hWnd = nullptr;
	m_fpsAccum = 0.0f;
	m_fpsFrames = 0;
}

DirectXBase::~DirectXBase()
{
}

bool DirectXBase::Init(HWND _hWnd, int _iWidth, int _iHeight)
{
	m_hWnd = _hWnd;
	m_iWindowWidth = _iWidth;
	m_iWindowHeight = _iHeight;

	WindowInit(); // 간단하게 윈도우 설정

	if (InitDirectX() == false) // DirectX 11 초기화
		return false;

	return true;
}

bool DirectXBase::InitDirectX()
{
	// 실제 클라이언트 영역 크기 (테두리 보정).
	RECT clientRect;
	GetClientRect(m_hWnd, &clientRect);
	const UINT width = clientRect.right - clientRect.left;
	const UINT height = clientRect.bottom - clientRect.top;

	UINT createFlags = 0;
#if defined(_DEBUG) || defined(WITH_EDITOR)
	createFlags |= D3D11_CREATE_DEVICE_DEBUG;
#endif

	DXGI_SWAP_CHAIN_DESC scd = {};
	scd.BufferCount = 1;
	scd.BufferDesc.Width = width;
	scd.BufferDesc.Height = height;
	scd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	scd.BufferDesc.RefreshRate.Numerator = 60;
	scd.BufferDesc.RefreshRate.Denominator = 1;
	scd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	scd.OutputWindow = m_hWnd;
	scd.SampleDesc.Count = 1;
	scd.SampleDesc.Quality = 0;
	scd.Windowed = TRUE;
	scd.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;

	const D3D_FEATURE_LEVEL requestedLevel = D3D_FEATURE_LEVEL_11_0;
	D3D_FEATURE_LEVEL obtainedLevel = {};

	HRESULT hr = D3D11CreateDeviceAndSwapChain(
		nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, createFlags,
		&requestedLevel, 1, D3D11_SDK_VERSION,
		&scd, &m_swapChain, &m_device, &obtainedLevel, &m_context);
	assert(SUCCEEDED(hr));
	if (FAILED(hr))
		return false;

	// 백버퍼 렌더 타깃 뷰.
	Microsoft::WRL::ComPtr<ID3D11Texture2D> backBuffer;
	hr = m_swapChain->GetBuffer(0, IID_PPV_ARGS(&backBuffer));
	assert(SUCCEEDED(hr));
	if (FAILED(hr))
		return false;

	hr = m_device->CreateRenderTargetView(backBuffer.Get(), nullptr, &m_backBufferRTV);
	assert(SUCCEEDED(hr));
	if (FAILED(hr))
		return false;

	// 깊이/스텐실 버퍼 (D24S8) + 뷰.
	D3D11_TEXTURE2D_DESC depthDesc = {};
	depthDesc.Width = width;
	depthDesc.Height = height;
	depthDesc.MipLevels = 1;
	depthDesc.ArraySize = 1;
	depthDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
	depthDesc.SampleDesc.Count = 1;
	depthDesc.SampleDesc.Quality = 0;
	depthDesc.Usage = D3D11_USAGE_DEFAULT;
	depthDesc.BindFlags = D3D11_BIND_DEPTH_STENCIL;

	hr = m_device->CreateTexture2D(&depthDesc, nullptr, &m_depthTex);
	assert(SUCCEEDED(hr));
	if (FAILED(hr))
		return false;

	hr = m_device->CreateDepthStencilView(m_depthTex.Get(), nullptr, &m_depthDSV);
	assert(SUCCEEDED(hr));
	if (FAILED(hr))
		return false;

	// 깊이 스테이트: 깊이 테스트 ON, 비교 LESS, 쓰기 ON.
	D3D11_DEPTH_STENCIL_DESC dsDesc = {};
	dsDesc.DepthEnable = TRUE;
	dsDesc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL;
	dsDesc.DepthFunc = D3D11_COMPARISON_LESS;
	dsDesc.StencilEnable = FALSE;
	hr = m_device->CreateDepthStencilState(&dsDesc, &m_depthState);
	assert(SUCCEEDED(hr));
	if (FAILED(hr))
		return false;

	D3D11_RASTERIZER_DESC rsDesc = {};
	rsDesc.FillMode = D3D11_FILL_SOLID;
	rsDesc.CullMode = D3D11_CULL_BACK;
	rsDesc.FrontCounterClockwise = FALSE;
	rsDesc.DepthClipEnable = TRUE;
	hr = m_device->CreateRasterizerState(&rsDesc, &m_rasterState);
	assert(SUCCEEDED(hr));
	if (FAILED(hr))
		return false;

	m_context->RSSetState(m_rasterState.Get());
	m_context->OMSetDepthStencilState(m_depthState.Get(), 0);

	// 뷰포트.
	D3D11_VIEWPORT viewport = {};
	viewport.TopLeftX = 0.0f;
	viewport.TopLeftY = 0.0f;
	viewport.Width = static_cast<float>(width);
	viewport.Height = static_cast<float>(height);
	viewport.MinDepth = 0.0f;
	viewport.MaxDepth = 1.0f;
	m_context->RSSetViewports(1, &viewport);

	return true;
}

void DirectXBase::RenderBegin(const float _clearColor[4])
{
	ID3D11RenderTargetView* rtv = m_backBufferRTV.Get();
	m_context->OMSetRenderTargets(1, &rtv, m_depthDSV.Get());
	m_context->ClearRenderTargetView(rtv, _clearColor);
	m_context->ClearDepthStencilView(m_depthDSV.Get(), D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);
}

void DirectXBase::RenderEnd()
{
	m_swapChain->Present(1, 0);
}

void DirectXBase::WindowInit()
{
	POINT centerPosition;
	centerPosition.x = GetSystemMetrics(SM_CXSCREEN) / 2;
	centerPosition.y = GetSystemMetrics(SM_CYSCREEN) / 2;
	centerPosition.x -= m_iWindowWidth / 2;
	centerPosition.y -= m_iWindowHeight / 2;

	RECT windowRect = { 
		centerPosition.x, 
		centerPosition.y, 
		centerPosition.x + m_iWindowWidth, 
		centerPosition.y + m_iWindowHeight 
	};
	
	AdjustWindowRect(&windowRect, WINDOW_STYLE, true);
	SetWindowPos(m_hWnd, nullptr, centerPosition.x, centerPosition.y, m_iWindowWidth, m_iWindowHeight, SWP_SHOWWINDOW);
}

void DirectXBase::MainLoop()
{
	const float dt = m_timer.Tick(); // 가변 delta time (초)

	UpdateGUI();

	Update(dt); // 로직 업데이트

	Render(); // 렌더링을 위한 세팅

	UpdateWindowTitleFps(dt);
}

void DirectXBase::UpdateWindowTitleFps(float _dt)
{
	m_fpsAccum += _dt;
	++m_fpsFrames;

	if (m_fpsAccum >= 1.0f) // 1초마다 갱신
	{
		const int fps = static_cast<int>(m_fpsFrames / m_fpsAccum + 0.5f);
		wchar_t title[128];
		swprintf_s(title, L"MiniEngine | FPS: %d | %.2f ms", fps, (m_fpsAccum / m_fpsFrames) * 1000.0f);
		SetWindowTextW(m_hWnd, title);

		m_fpsAccum = 0.0f;
		m_fpsFrames = 0;
	}
}
