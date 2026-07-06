#pragma once

namespace MiniEngine
{
	class World;
	class SceneManager
	{
		SINGLETON(SceneManager)

	public:
		void Init();

		void BeginPlay();
		void Update(float _dt);
		void Render();

		std::weak_ptr<World> GetCurrentScene() const { return m_pCurScene; }

	private:
		std::shared_ptr<World> m_pCurScene{ nullptr };
	};
}