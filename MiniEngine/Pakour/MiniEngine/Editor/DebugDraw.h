#pragma once
#include <vector>
#include "Core/Math.h"
#include "Core/DebugLine.h"

struct ID3D11Device;
struct ID3D11DeviceContext;

// 디버그 라인 렌더러. **Debug/Editor 구성에서만 실제로 동작**하며 Release(MG_RELEASE)에서는
// 전부 no-op 으로 컴파일된다(D3D 리소스/셰이더 미참조). Core/Log.h 와 동일한 격리 관용구. (§4)
#if !defined(MG_RELEASE)

#include <d3d11.h>
#include <wrl/client.h>

namespace MiniEngine
{
    // LINELIST 토폴로지로 DebugLine 배열을 한 번에 그린다(선 하나 = 정점 2개).
    // 래스터라이저/깊이 상태는 새로 만들지 않는다 — 선은 컬링 대상이 아니라
    // DirectXBase 가 이미 바인딩해 둔 상태를 그대로 쓴다. 따라서 선은 깊이 테스트를 받는다
    // (메시 뒤에 있으면 가려진다).
    class DebugDrawer
    {
    public:
        // DebugLine.hlsl 컴파일 + 입력레이아웃 + 동적 VB/CB 생성. 성공 시 true.
        bool Init(ID3D11Device* _device, ID3D11DeviceContext* _context);

        // _lines 를 현재 바인딩된 RTV/DSV 에 그린다. _viewProj = view * proj (전치 없음).
        // 비어 있으면 아무 상태도 건드리지 않고 즉시 반환.
        void Draw(const std::vector<DebugLine>& _lines, const Matrix& _viewProj);

    private:
        // 정점 _count 개를 담을 수 있도록 VB 를 확보(부족하면 2배씩 키워 재생성). 실패 시 false.
        bool EnsureVertexCapacity(size_t _count);

        ID3D11Device*        m_device  = nullptr; // 비소유 — DirectXBase 소유. (§12)
        ID3D11DeviceContext* m_context = nullptr; // 비소유

        Microsoft::WRL::ComPtr<ID3D11VertexShader> m_vertexShader;
        Microsoft::WRL::ComPtr<ID3D11PixelShader>  m_pixelShader;
        Microsoft::WRL::ComPtr<ID3D11InputLayout>  m_inputLayout;
        Microsoft::WRL::ComPtr<ID3D11Buffer>       m_vertexBuffer; // DYNAMIC, 매 프레임 WRITE_DISCARD
        Microsoft::WRL::ComPtr<ID3D11Buffer>       m_frameCB;      // b0: viewProj (DYNAMIC)

        size_t m_vertexCapacity = 0; // m_vertexBuffer 가 담을 수 있는 정점 수
    };
}

#else // MG_RELEASE — no-op 스텁(호출부에 #if 가 퍼지지 않게 시그니처는 동일)

namespace MiniEngine
{
    class DebugDrawer
    {
    public:
        bool Init(ID3D11Device*, ID3D11DeviceContext*) { return true; }
        void Draw(const std::vector<DebugLine>&, const Matrix&) {}
    };
}

#endif
