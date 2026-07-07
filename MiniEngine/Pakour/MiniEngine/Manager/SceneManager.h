#pragma once

namespace MiniEngine
{
	namespace Graphics { struct RenderContext; }
	
	class Scene;
	class SceneManager
	{
		SINGLETON(SceneManager)

	public:
		void Init();

		void BeginPlay();
		void FixedUpdate(float _dt);
		void Update(float _dt);
		void Render(Graphics::RenderContext& _context);
		void EndPlay();

		std::weak_ptr<Scene> GetCurrentScene() const { return m_pCurScene; }

	private:
		std::shared_ptr<Scene> m_pCurScene{ nullptr };
	};
}