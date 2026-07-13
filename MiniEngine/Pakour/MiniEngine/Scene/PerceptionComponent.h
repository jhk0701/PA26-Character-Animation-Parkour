#pragma once
#include "Scene/Component.h"
#include <functional>
#include <vector>

namespace MiniEngine 
{
	namespace Physics { class PhysicsWorld; }

	// 장애물 충돌 시 우회하는 방향
	enum class EDirection : uint8_t
	{
		UP,
		RIGHT,
		DOWN,
		LEFT,

		NONE
	};

	struct TravelResult 
	{
		EDirection m_dir;
		Vector3 m_pos;
	};

	class PerceptionComponent : public Component
	{
	public:
		void OnAttach() override;
		void Tick(float _dt) override;
		void StartTravel(); // 탐색

	private:
		std::weak_ptr<Physics::PhysicsWorld> m_physics;
		
		float m_unit{ 1.0f }; // 탐색 단위

		// 1. 평지 이동 시, 장애물 탐색
		// 탐색 범위 
		float m_maxObsDist{ 2.0f };
		float m_maxLandDist{ 1000.0f }; // 바닥 탐색


	};
}