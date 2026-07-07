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

		const Matrix world = GetWorldMatrix();
		
	}
}
