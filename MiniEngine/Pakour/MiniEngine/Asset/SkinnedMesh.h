#pragma once
#include <vector>
#include <cstdint>
#include <d3d11.h>
#include <wrl/client.h>
#include "Asset/MiniFormat.h"
#include "Asset/Skeleton.h"
#include "Asset/AnimClip.h"
#include "Asset/BoneNaming.h"

namespace MiniEngine
{
    // 스키닝 메시
    class SkinnedMesh
    {
    public:
        // 렌더 정점 포맷. MiniSkinnedVertex 와 동일 레이아웃(pos3/normal3/uv2 + bone4/weight4).
        using Vertex = MiniSkinnedVertex;

        SkinnedMesh() = default;

        // CPU 데이터 설정. GPU 리소스 생성 전에 호출.
        void SetData(std::vector<Vertex> _vertices, std::vector<uint32_t> _indices);
        void SetSkeleton(Skeleton _skeleton); // 루트 모션 본 + 휴머노이드 본 확인
        void SetClips(std::vector<AnimClip> _clips) { m_clips = std::move(_clips); }

        // 루트 모션을 방출하는 본("root" 또는 Hips 역할). -1 = 없음(루트 모션 무동작).
        // SetSkeleton 이 FindRootMotionBone 으로 채우며, 필요 시 호출자가 덮어쓸 수 있다.
        int  GetRootMotionBone() const { return m_rootMotionBone; }
        void SetRootMotionBone(int _boneIndex) { m_rootMotionBone = _boneIndex; }
        const HumanoidBoneMap& GetHumanoidBones() const { return m_humanoidBones; }

        uint32_t GetBakeFlags() const { return m_bakeFlags; }
        void     SetBakeFlags(uint32_t _flags) { m_bakeFlags = _flags; }

        // CPU 데이터로부터 GPU 정점/인덱스 버퍼를 생성. 성공 시 true.
        bool CreateGpuResources(ID3D11Device* _device);

        // ---- 렌더용 접근자 ----
        ID3D11Buffer* GetVertexBuffer() const { return m_vertexBuffer.Get(); }
        ID3D11Buffer* GetIndexBuffer()  const { return m_indexBuffer.Get(); }
        uint32_t      GetIndexCount()   const { return m_indexCount; }
        uint32_t      GetVertexStride() const { return static_cast<uint32_t>(sizeof(Vertex)); }
        bool          HasGpuResources() const { return m_vertexBuffer && m_indexBuffer; }

        const std::vector<Vertex>& GetVertices() const { return m_vertices; }
        const std::vector<uint32_t>& GetIndices()  const { return m_indices; }
        const Skeleton& GetSkeleton() const { return m_skeleton; }
        const std::vector<AnimClip>& GetClips()    const { return m_clips; }
        AnimClip* GetClipPtr(int _idx);

    private:
        std::vector<Vertex>   m_vertices;
        std::vector<uint32_t> m_indices;
        Skeleton              m_skeleton;
        std::vector<AnimClip> m_clips;
        int                   m_rootMotionBone = -1;
        HumanoidBoneMap       m_humanoidBones;  // SetSkeleton 이 BuildHumanoidBoneMap 으로 채운다.
        uint32_t              m_bakeFlags = 0;

        Microsoft::WRL::ComPtr<ID3D11Buffer> m_vertexBuffer;
        Microsoft::WRL::ComPtr<ID3D11Buffer> m_indexBuffer;
        uint32_t m_indexCount = 0;
    };
}
