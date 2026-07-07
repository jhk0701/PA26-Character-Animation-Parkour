#include "pch.h"
#include "Scene/StaticMeshComponent.h"
#include "Core/Graphics.h"

// StaticMeshComponent 는 현재 인라인(헤더) 구현만 가진다.
// 향후 렌더 상태/머티리얼 확장 시 이 파일에 비인라인 로직을 추가한다.
namespace MiniEngine
{
	void StaticMeshComponent::Render(Graphics::RenderContext& _context)
	{
		SceneComponent::Render(_context);

		if (!m_mesh || !m_mesh->HasGpuResources())
			return;

		ID3D11DeviceContext*& pContext = _context.m_context;

		Graphics::PerObjectCB perObject = {};
		perObject.world = GetWorldMatrix();
		perObject.mvp = perObject.world * _context.m_camView * _context.m_camProj; // model view projection 연산

		D3D11_MAPPED_SUBRESOURCE mapped = {};
		if (SUCCEEDED(pContext->Map(
			_context.m_perObjectCB, 0, 
			D3D11_MAP_WRITE_DISCARD, 0, 
			&mapped)))
		{
			memcpy(mapped.pData, &perObject, sizeof(perObject));
			pContext->Unmap(_context.m_perObjectCB, 0);
		}

		if (SUCCEEDED(pContext->Map(
			_context.m_perFrameCB, 0,
			D3D11_MAP_WRITE_DISCARD, 0,
			&mapped))) 
		{
			memcpy(mapped.pData, &_context.m_perFrame, sizeof(_context.m_perFrame));
			pContext->Unmap(_context.m_perFrameCB, 0);
		}

		UINT stride = m_mesh->GetVertexStride();
		UINT offset = 0;
		ID3D11Buffer* vb = m_mesh->GetVertexBuffer();

		pContext->IASetVertexBuffers(0, 1, &vb, &stride, &offset);
		pContext->IASetIndexBuffer(m_mesh->GetIndexBuffer(), DXGI_FORMAT_R32_UINT, 0);
		pContext->DrawIndexed(m_mesh->GetIndexCount(), 0, 0);
	}
}
