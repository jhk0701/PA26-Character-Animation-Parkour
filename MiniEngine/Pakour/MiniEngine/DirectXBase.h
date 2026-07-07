#pragma once
#include <d3d11.h>
#include <d3dcompiler.h>
#include <wrl.h> // ComPtr
#include "Core/Timer.h"

// direct X를 사용하기 위해서 초기 설정 작업이 있음
// 그러므로 이를 수행할 클래스를 만들어서 담당시켜 작업을 해줄 것
// 필요한 곳에서는 상속해서 사용
class DirectXBase
{
private:
	//Windows
	HWND m_hWnd;

	void WindowInit();

	// 1초마다 창 타이틀에 FPS 표시 (검증/디버그용).
	void UpdateWindowTitleFps(float _dt);
	float m_fpsAccum;   // FPS 갱신용 누적 시간
	int   m_fpsFrames;  // 누적 프레임 수

protected:
	// Windows
	int m_iWindowWidth;
	int m_iWindowHeight;

	// 프레임 타이머 (가변 dt)
	MiniEngine::Timer m_timer;

	// DirectX 11 코어 객체
	Microsoft::WRL::ComPtr<ID3D11Device>           m_device;
	Microsoft::WRL::ComPtr<ID3D11DeviceContext>    m_context;
	Microsoft::WRL::ComPtr<IDXGISwapChain>         m_swapChain;
	Microsoft::WRL::ComPtr<ID3D11RenderTargetView> m_backBufferRTV;

	// 깊이/스텐실 (3D 렌더용). D24_UNORM_S8_UINT.
	Microsoft::WRL::ComPtr<ID3D11Texture2D>         m_depthTex;
	Microsoft::WRL::ComPtr<ID3D11DepthStencilView>  m_depthDSV;
	Microsoft::WRL::ComPtr<ID3D11DepthStencilState> m_depthState;   // LESS
	Microsoft::WRL::ComPtr<ID3D11RasterizerState>   m_rasterState;  // CULL_BACK, CW=front (LH)

	// Device / SwapChain / RTV / DepthStencil / Viewport 초기화. 실패 시 false.
	bool InitDirectX();

	// 백버퍼+깊이버퍼를 렌더 타깃으로 바인딩하고 컬러/깊이를 클리어.
	void RenderBegin(const float _clearColor[4]);
	// 백버퍼를 화면에 표시.
	void RenderEnd();

public:
	DirectXBase();
	virtual ~DirectXBase();

	void MainLoop();
	virtual bool Init(HWND _hWnd, int _iWidth, int _iHeight) abstract;
	virtual void Update(float _dt) = 0;
	virtual void Render() = 0;
	virtual void UpdateGUI() = 0;
};

