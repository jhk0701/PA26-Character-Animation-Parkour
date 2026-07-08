#pragma once
#include <string>
#include "DirectXBase.h"
#include "Editor/EditorUI.h"

// 테스트용 전방선언
namespace MiniEngine { class CameraComponent; }

class GameCore : public DirectXBase
{
public:
	GameCore();
	~GameCore();

	// DirectXBase을(를) 통해 상속됨
	bool Init(HWND _hWnd, int _iWidth, int _iHeight) override;

	void BeginPlay();
	void Update(float _dt) override;
	void Render() override;
	void UpdateGUI() override;
	void EndPlay();

	void QuitGame();

private:
	// Lambert 셰이더 + 상수버퍼 생성.
	bool InitRenderResources();
	// 주어진 .mini 를 로드해 씬에 메시 Actor 로 스폰(Baker "Bake & Load" 소비).
	bool SpawnMeshFromMini(const std::wstring& _miniPath);

	// 에디터 UI
	MiniEngine::Editor::EditorUI				m_editor;

	// 렌더 리소스
	Microsoft::WRL::ComPtr<ID3D11VertexShader> m_vertexShader;
	Microsoft::WRL::ComPtr<ID3D11PixelShader>  m_pixelShader;
	Microsoft::WRL::ComPtr<ID3D11InputLayout>  m_inputLayout;
	Microsoft::WRL::ComPtr<ID3D11Buffer>       m_perObjectCB; // b0: mvp + world (DYNAMIC)
	Microsoft::WRL::ComPtr<ID3D11Buffer>       m_perFrameCB;  // b1: 라이트/알베도 (DYNAMIC)

	// GPU 스키닝 렌더 리소스 (SkinnedMeshLambert.hlsl, b0/b1 공유 + b2 본 행렬)
	Microsoft::WRL::ComPtr<ID3D11VertexShader> m_skinnedVertexShader;
	Microsoft::WRL::ComPtr<ID3D11PixelShader>  m_skinnedPixelShader;
	Microsoft::WRL::ComPtr<ID3D11InputLayout>  m_skinnedInputLayout;
	Microsoft::WRL::ComPtr<ID3D11Buffer>       m_boneCB;      // b2: 본 최종 행렬 128개 (DYNAMIC)
};
