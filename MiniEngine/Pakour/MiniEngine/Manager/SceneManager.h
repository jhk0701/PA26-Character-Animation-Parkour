#pragma once
#include <d3d11.h>

namespace MiniEngine
{
	namespace Graphics { struct RenderContext; }
	
	class Scene;
	class SceneManager
	{
		SINGLETON(SceneManager)

	public:
		void Init(ID3D11Device* _device, ID3D11DeviceContext* _context);

		void BeginPlay();
		void FixedUpdate(float _dt);
		void Update(float _dt);
		void LateUpdate(float _dt);

		void Render(Graphics::RenderContext& _context);
		void EndPlay();

		std::weak_ptr<Scene> GetCurrentScene() const { return m_pCurScene; }

	private:
		std::shared_ptr<Scene> m_pCurScene{ nullptr };
	};
}