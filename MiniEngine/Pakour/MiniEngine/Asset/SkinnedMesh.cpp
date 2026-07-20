#include "pch.h"
#include "Asset/SkinnedMesh.h"
#include "Core/Log.h"

namespace MiniEngine
{
    void SkinnedMesh::SetData(std::vector<Vertex> _vertices, std::vector<uint32_t> _indices)
    {
        m_vertices = std::move(_vertices);
        m_indices = std::move(_indices);
    }

    void SkinnedMesh::SetSkeleton(Skeleton _skeleton)
    {
        m_skeleton = std::move(_skeleton);
        m_rootMotionBone = FindRootMotionBone(m_skeleton);
        BuildHumanoidBoneMap(m_skeleton, m_humanoidBones); 
    }

    bool SkinnedMesh::CreateGpuResources(ID3D11Device* _device)
    {
        if (_device == nullptr)
            return false;
        if (m_vertices.empty() || m_indices.empty())
            return false;

        // Vertex Buffer (IMMUTABLE).
        D3D11_BUFFER_DESC vbDesc = {};
        vbDesc.Usage = D3D11_USAGE_IMMUTABLE;
        vbDesc.ByteWidth = static_cast<UINT>(m_vertices.size() * sizeof(Vertex));
        vbDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
        D3D11_SUBRESOURCE_DATA vbData = {};
        vbData.pSysMem = m_vertices.data();
        HRESULT hr = _device->CreateBuffer(&vbDesc, &vbData, &m_vertexBuffer);
        if (FAILED(hr))
            return false;

        // Index Buffer (IMMUTABLE, R32_UINT).
        D3D11_BUFFER_DESC ibDesc = {};
        ibDesc.Usage = D3D11_USAGE_IMMUTABLE;
        ibDesc.ByteWidth = static_cast<UINT>(m_indices.size() * sizeof(uint32_t));
        ibDesc.BindFlags = D3D11_BIND_INDEX_BUFFER;
        D3D11_SUBRESOURCE_DATA ibData = {};
        ibData.pSysMem = m_indices.data();
        hr = _device->CreateBuffer(&ibDesc, &ibData, &m_indexBuffer);
        if (FAILED(hr))
        {
            m_vertexBuffer.Reset();
            return false;
        }

        m_indexCount = static_cast<uint32_t>(m_indices.size());
        return true;
    }

    AnimClip* SkinnedMesh::GetClipPtr(int _idx)
    {
        if (_idx >= m_clips.size())
        {
            MG_LOG_ERROR("[SkinnedMesh] Animation is not exists at {} index", _idx);
            return nullptr;
        }
            
        return &m_clips[_idx];
    }
}
