#pragma once
#include <vector>
#include <cstdint>
#include <d3d11.h>
#include <wrl/client.h>
#include "Asset/MiniFormat.h"

namespace MiniEngine
{
    // 정적 메시. CPU 정점/인덱스 데이터를 보유하고, 요청 시 GPU 버퍼(IMMUTABLE)를 생성한다.
    // 소유는 AssetManager 캐시(weak) + 사용처(StaticMeshComponent, shared). (§12)
    class StaticMesh
    {
    public:
        // 렌더 정점 포맷. MiniStaticVertex 와 동일 레이아웃(pos3/normal3/uv2).
        using Vertex = MiniStaticVertex;

        StaticMesh() = default;

        // CPU 데이터 설정(로더가 채운다). GPU 리소스 생성 전에 호출.
        void SetData(std::vector<Vertex> _vertices, std::vector<uint32_t> _indices);

        // CPU 데이터로부터 GPU 정점/인덱스 버퍼를 생성. 성공 시 true.
        bool CreateGpuResources(ID3D11Device* _device);

        // ---- 렌더용 접근자 ----
        ID3D11Buffer* GetVertexBuffer() const { return m_vertexBuffer.Get(); }
        ID3D11Buffer* GetIndexBuffer()  const { return m_indexBuffer.Get(); }
        uint32_t      GetIndexCount()   const { return m_indexCount; }
        uint32_t      GetVertexStride() const { return static_cast<uint32_t>(sizeof(Vertex)); }
        bool          HasGpuResources() const { return m_vertexBuffer && m_indexBuffer; }

        const std::vector<Vertex>&   GetVertices() const { return m_vertices; }
        const std::vector<uint32_t>& GetIndices()  const { return m_indices; }

    private:
        std::vector<Vertex>   m_vertices;
        std::vector<uint32_t> m_indices;

        Microsoft::WRL::ComPtr<ID3D11Buffer> m_vertexBuffer;
        Microsoft::WRL::ComPtr<ID3D11Buffer> m_indexBuffer;
        uint32_t m_indexCount = 0;
    };
}
