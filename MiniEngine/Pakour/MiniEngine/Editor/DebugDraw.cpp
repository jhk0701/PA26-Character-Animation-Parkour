#include "pch.h"
#include "Editor/DebugDraw.h"

#if !defined(MG_RELEASE)

#include "Asset/ShaderUtil.h"
#include "Core/Log.h"
#include <cassert>

namespace MiniEngine
{
    namespace
    {
        // 정점 레이아웃 = DebugLine 의 끝점 하나. 16바이트.
        struct DebugVertex
        {
            Vector3  position;
            uint32_t colorARGB; // 0xAARRGGBB — B8G8R8A8_UNORM 과 바이트 일치(Core/DebugLine.h)
        };

        // b0. 정점이 이미 월드 공간이라 world 행렬은 없다.
        struct DebugFrameCB
        {
            Matrix viewProj;
        };

        constexpr size_t INITIAL_VERTEX_CAPACITY = 16384; // 8192 라인 = 256KB
    }

    bool DebugDrawer::Init(ID3D11Device* _device, ID3D11DeviceContext* _context)
    {
        assert(_device && _context);
        m_device  = _device;
        m_context = _context;

        const std::wstring shaderPath = ShaderUtil::ResolvePath(L"DebugLine.hlsl");

        Microsoft::WRL::ComPtr<ID3DBlob> vsBlob;
        HRESULT hr = ShaderUtil::CompileFromFile(shaderPath, "VSMain", "vs_5_0", &vsBlob);
        assert(SUCCEEDED(hr));
        if (FAILED(hr)) return false;

        hr = m_device->CreateVertexShader(vsBlob->GetBufferPointer(), vsBlob->GetBufferSize(), nullptr, &m_vertexShader);
        assert(SUCCEEDED(hr));
        if (FAILED(hr)) return false;

        // POSITION(0) / COLOR(12) — DebugVertex 레이아웃과 일치.
        // B8G8R8A8_UNORM 은 메모리 바이트 (B,G,R,A) 를 읽어 셰이더에 (R,G,B,A) 로 준다.
        // 0xAARRGGBB 를 리틀엔디언 uint32 로 저장하면 바이트가 정확히 (B,G,R,A) → 변환 불필요.
        const D3D11_INPUT_ELEMENT_DESC layout[] =
        {
            { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0,  D3D11_INPUT_PER_VERTEX_DATA, 0 },
            { "COLOR",    0, DXGI_FORMAT_B8G8R8A8_UNORM,  0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0 },
        };
        hr = m_device->CreateInputLayout(layout, ARRAYSIZE(layout), vsBlob->GetBufferPointer(), vsBlob->GetBufferSize(), &m_inputLayout);
        assert(SUCCEEDED(hr));
        if (FAILED(hr)) return false;

        Microsoft::WRL::ComPtr<ID3DBlob> psBlob;
        hr = ShaderUtil::CompileFromFile(shaderPath, "PSMain", "ps_5_0", &psBlob);
        assert(SUCCEEDED(hr));
        if (FAILED(hr)) return false;

        hr = m_device->CreatePixelShader(psBlob->GetBufferPointer(), psBlob->GetBufferSize(), nullptr, &m_pixelShader);
        assert(SUCCEEDED(hr));
        if (FAILED(hr)) return false;

        // per-frame 상수버퍼 (DYNAMIC). viewProj = 64바이트.
        D3D11_BUFFER_DESC cbDesc = {};
        cbDesc.Usage          = D3D11_USAGE_DYNAMIC;
        cbDesc.ByteWidth      = sizeof(DebugFrameCB);
        cbDesc.BindFlags      = D3D11_BIND_CONSTANT_BUFFER;
        cbDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
        hr = m_device->CreateBuffer(&cbDesc, nullptr, &m_frameCB);
        assert(SUCCEEDED(hr));
        if (FAILED(hr)) return false;

        if (!EnsureVertexCapacity(INITIAL_VERTEX_CAPACITY))
            return false;

        MG_LOG_INFO("DebugDrawer initialized (line capacity {})", INITIAL_VERTEX_CAPACITY / 2);
        return true;
    }

    bool DebugDrawer::EnsureVertexCapacity(size_t _count)
    {
        if (_count <= m_vertexCapacity)
            return true;

        // 2배씩 키워 재할당 횟수를 로그로 억제.
        size_t newCapacity = (m_vertexCapacity > 0) ? m_vertexCapacity : INITIAL_VERTEX_CAPACITY;
        while (newCapacity < _count)
            newCapacity *= 2;

        D3D11_BUFFER_DESC vbDesc = {};
        vbDesc.Usage          = D3D11_USAGE_DYNAMIC;
        vbDesc.ByteWidth      = static_cast<UINT>(newCapacity * sizeof(DebugVertex));
        vbDesc.BindFlags      = D3D11_BIND_VERTEX_BUFFER;
        vbDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;

        Microsoft::WRL::ComPtr<ID3D11Buffer> newBuffer;
        HRESULT hr = m_device->CreateBuffer(&vbDesc, nullptr, &newBuffer);
        if (FAILED(hr))
        {
            MG_LOG_ERROR("DebugDrawer: vertex buffer resize to {} verts failed", newCapacity);
            return false;
        }

        m_vertexBuffer   = newBuffer; // 이전 버퍼는 ComPtr 이 release
        m_vertexCapacity = newCapacity;
        return true;
    }

    void DebugDrawer::Draw(const std::vector<DebugLine>& _lines, const Matrix& _viewProj)
    {
        if (!m_context || _lines.empty())
            return;

        const size_t vertexCount = _lines.size() * 2;
        if (!EnsureVertexCapacity(vertexCount))
            return;

        // 정점 업로드. 선 하나 = 양 끝점 2개(같은 색).
        D3D11_MAPPED_SUBRESOURCE mapped = {};
        if (FAILED(m_context->Map(m_vertexBuffer.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped)))
            return;
        {
            auto* dst = static_cast<DebugVertex*>(mapped.pData);
            for (const DebugLine& line : _lines)
            {
                *dst++ = { line.a, line.colorARGB };
                *dst++ = { line.b, line.colorARGB };
            }
        }
        m_context->Unmap(m_vertexBuffer.Get(), 0);

        DebugFrameCB frame = {};
        frame.viewProj = _viewProj;
        if (SUCCEEDED(m_context->Map(m_frameCB.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped)))
        {
            memcpy(mapped.pData, &frame, sizeof(frame));
            m_context->Unmap(m_frameCB.Get(), 0);
        }

        UINT stride = sizeof(DebugVertex);
        UINT offset = 0;
        ID3D11Buffer* vb      = m_vertexBuffer.Get();
        ID3D11Buffer* frameCB = m_frameCB.Get();

        m_context->IASetInputLayout(m_inputLayout.Get());
        m_context->IASetVertexBuffers(0, 1, &vb, &stride, &offset);
        m_context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_LINELIST);
        m_context->VSSetShader(m_vertexShader.Get(), nullptr, 0);
        m_context->VSSetConstantBuffers(0, 1, &frameCB);
        m_context->PSSetShader(m_pixelShader.Get(), nullptr, 0);
        m_context->Draw(static_cast<UINT>(vertexCount), 0);
    }
}

#endif // !MG_RELEASE
