#pragma once
#include <string>
#include "DirectXBase.h"
#include "Platform/Input.h"
#include "Scene/World.h"
#include "Scene/CameraComponent.h"
#include "Scene/CameraController.h"
#include "Scene/StaticMeshComponent.h"
#include "Scene/SkeletalMeshComponent.h"
#include "Asset/AssetManager.h"
#include "Editor/EditorUI.h"

class GameCore : public DirectXBase
{
public:
	GameCore();
	~GameCore();

	// DirectXBase을(를) 통해 상속됨
	bool Init(HWND _hWnd, int _iWidth, int _iHeight) override;
	void Update(float _dt) override;
	void Render() override;
	void UpdateGUI() override;

private:
	// Lambert 셰이더 + 상수버퍼 생성.
	bool InitRenderResources();
	// .mini StaticMesh(없으면 절차 생성) 로드 → 메시 Actor 스폰.
	bool InitMeshScene();
	// .mini SkinnedMesh(없으면 절차적 2-본 박스 생성) 로드 → 스키닝 Actor 스폰.
	bool InitSkinnedScene();
	// 주어진 .mini 를 로드해 씬에 메시 Actor 로 스폰(Baker "Bake & Load" 소비).
	bool SpawnMeshFromMini(const std::wstring& _miniPath);
	// 선택 카메라로 메시 컴포넌트를 Lambert 셰이딩으로 그린다.
	void DrawMesh(MiniEngine::CameraComponent& _camera, MiniEngine::StaticMeshComponent& _meshComp);
	// 선택 카메라로 스키닝 메시를 GPU 스키닝 + Lambert 로 그린다(본 행렬 b2 업로드).
	void DrawSkinnedMesh(MiniEngine::CameraComponent& _camera, MiniEngine::SkeletalMeshComponent& _meshComp);
	// 마우스 위치 → 월드 레이 → StaticMesh AABB 교차. 히트 Actor 인덱스(World::GetActors 기준), 미스는 -1.
	int  PickActor(const MiniEngine::CameraComponent& _camera) const;

	MiniEngine::Input m_input;

	// 씬. 소유자는 World(m_actors). 메시 핸들은 비소유(weak) 캐시.
	// 카메라 컴포넌트도 비소유 캐시(weak). (§12)
	MiniEngine::World                              m_world;
	MiniEngine::AssetManager                       m_assets;
	std::weak_ptr<MiniEngine::Actor>               m_meshActor;
	std::weak_ptr<MiniEngine::StaticMeshComponent> m_meshComponent;
	std::shared_ptr<MiniEngine::Actor>             m_cameraActor;
	std::weak_ptr<MiniEngine::CameraComponent>     m_camera;
	MiniEngine::CameraController                   m_camController;
	float                                          m_meshAngle = 0.0f;

	// 에디터 UI (Editor 구성 전용 동작, 그 외 no-op). §4/§14.2
	MiniEngine::Editor::EditorUI                   m_editor;

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
