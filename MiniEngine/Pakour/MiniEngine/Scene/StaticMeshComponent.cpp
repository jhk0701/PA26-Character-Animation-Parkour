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

		// TODO : 객체별 색상 받아오기
		_context.m_perFrame.albedo = Vector3(0.3f);

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

		pContext->IASetInputLayout(_context.m_staticMeshInputLayout);
		pContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
		pContext->IASetVertexBuffers(0, 1, &vb, &stride, &offset);
		pContext->IASetIndexBuffer(m_mesh->GetIndexBuffer(), DXGI_FORMAT_R32_UINT, 0);
		pContext->VSSetShader(_context.m_staticMeshVS, nullptr, 0);
		pContext->VSSetConstantBuffers(0, 1, &_context.m_perObjectCB);
		pContext->PSSetShader(_context.m_staticMeshPS, nullptr, 0);
		pContext->PSSetConstantBuffers(1, 1, &_context.m_perFrameCB);
		pContext->DrawIndexed(m_mesh->GetIndexCount(), 0, 0);
	}
}


// 선택된 카메라로 메시 컴포넌트를 Lambert 셰이딩으로 그린다.
//void GameCore::DrawMesh(MiniEngine::CameraComponent& _camera, MiniEngine::StaticMeshComponent& _meshComp)
//{
//    auto mesh = _meshComp.GetMesh();
//
//    // MVP = world * view * proj (row-vector, 전치 없음 — 셰이더 cbuffer는 row_major).
//    const Matrix world = _meshComp.GetWorldMatrix();
//    const Matrix view = _camera.GetViewMatrix();
//    const Matrix proj = _camera.GetProjectionMatrix();
//
//    PerObjectCB perObject = {};
//    perObject.mvp = world * view * proj;
//    perObject.world = world;
//
//    D3D11_MAPPED_SUBRESOURCE mapped = {};
//    if (SUCCEEDED(m_context->Map(m_perObjectCB.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped)))
//    {
//        memcpy(mapped.pData, &perObject, sizeof(perObject));
//        m_context->Unmap(m_perObjectCB.Get(), 0);
//    }
//
//    // per-frame 라이트/알베도. lightDir = 빛이 나아가는 방향(정규화). PS 에서 -l 로 N·L 계산.
//    // 씬에서 계산
//    PerFrameCB perFrame = {};
//    /*Vector3 dir(-0.4f, -1.0f, 0.6f);
//    dir.Normalize();
//    perFrame.lightDir = dir;
//    perFrame.ambient = 0.15f;
//    perFrame.lightColor = Vector3(1.0f, 1.0f, 1.0f);
//    perFrame.albedo = Vector3(0.85f, 0.78f, 0.70f);*/
//
//if (SUCCEEDED(m_context->Map(
//    m_perFrameCB.Get(), 0,
//    D3D11_MAP_WRITE_DISCARD, 0,
//    &mapped)))
//{
//    memcpy(mapped.pData, &perFrame, sizeof(perFrame));
//    m_context->Unmap(m_perFrameCB.Get(), 0);
//}
//
//UINT stride = mesh->GetVertexStride();
//UINT offset = 0;
//ID3D11Buffer* vb = mesh->GetVertexBuffer();
//ID3D11Buffer* objectCB = m_perObjectCB.Get();
//ID3D11Buffer* frameCB = m_perFrameCB.Get();
//
//m_context->IASetInputLayout(m_inputLayout.Get());
//m_context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
//
//m_context->IASetVertexBuffers(0, 1, &vb, &stride, &offset);
//m_context->IASetIndexBuffer(mesh->GetIndexBuffer(), DXGI_FORMAT_R32_UINT, 0);
//m_context->VSSetShader(m_vertexShader.Get(), nullptr, 0);
//m_context->VSSetConstantBuffers(0, 1, &objectCB);
//m_context->PSSetShader(m_pixelShader.Get(), nullptr, 0);
//m_context->PSSetConstantBuffers(1, 1, &frameCB);
//m_context->DrawIndexed(mesh->GetIndexCount(), 0, 0);
//}
