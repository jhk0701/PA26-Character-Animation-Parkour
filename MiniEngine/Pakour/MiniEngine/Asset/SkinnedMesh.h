#pragma once
#include <vector>
#include <cstdint>
#include <d3d11.h>
#include <wrl/client.h>
#include "Asset/MiniFormat.h"
#include "Asset/Skeleton.h"
#include "Asset/AnimClip.h"

namespace MiniEngine
{
    // 스키닝 메시 (통합 컨테이너 — 정점/인덱스 + 스켈레톤 + AnimClip). (CLAUDE.md §8/§9)
    // StaticMesh 와 동형: CPU 데이터 보유 + 요청 시 GPU 버퍼(IMMUTABLE) 생성.
    // 소유는 AssetManager 캐시(weak) + 사용처(SkeletalMeshComponent, shared). (§12)
    class SkinnedMesh
    {
    public:
        // 렌더 정점 포맷. MiniSkinnedVertex 와 동일 레이아웃(pos3/normal3/uv2 + bone4/weight4).
        using Vertex = MiniSkinnedVertex;

        SkinnedMesh() = default;

        // CPU 데이터 설정(로더가 채운다). GPU 리소스 생성 전에 호출.
        void SetData(std::vector<Vertex> _vertices, std::vector<uint32_t> _indices);
        void SetSkeleton(Skeleton _skeleton) { m_skeleton = std::move(_skeleton); }
        void SetClips(std::vector<AnimClip> _clips) { m_clips = std::move(_clips); }

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
        const Skeleton&              GetSkeleton() const { return m_skeleton; }
        const std::vector<AnimClip>& GetClips()    const { return m_clips; }

    private:
        std::vector<Vertex>   m_vertices;
        std::vector<uint32_t> m_indices;
        Skeleton              m_skeleton;
        std::vector<AnimClip> m_clips;

        Microsoft::WRL::ComPtr<ID3D11Buffer> m_vertexBuffer;
        Microsoft::WRL::ComPtr<ID3D11Buffer> m_indexBuffer;
        uint32_t m_indexCount = 0;
    };
}
