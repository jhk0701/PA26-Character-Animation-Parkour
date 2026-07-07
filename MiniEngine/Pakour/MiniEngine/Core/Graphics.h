#pragma once
#include <d3d11.h>
#include <wrl/client.h>
#include "Core/Math.h"

struct ID3D11DeviceContext;
struct ID3D11Buffer;

namespace MiniEngine::Graphics
{
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

    // 게임루프 렌더 시, 넘겨줄 context 구조체
    struct RenderContext 
    {
        ID3D11DeviceContext* m_context;
        ID3D11Buffer* m_perObjectCB;
        ID3D11Buffer* m_perFrameCB;
        
        Matrix m_camView;
        Matrix m_camProj;
        PerFrameCB m_perFrame;
    };
}