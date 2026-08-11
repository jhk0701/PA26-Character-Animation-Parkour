#pragma once
#include "Core/Math.h"

namespace MiniEngine 
{
	class IObstacle;
	struct PerceptedObstacleInfo
	{
		IObstacle* m_pObstacle{ nullptr };
		bool m_bIsNewObstacle{ true };
		bool m_bDetectLedge{ false };
		float m_obstacleDistance{ 0.0f };
		float m_obstacleLedge{ 0.0f };
		float m_obstacleDepth{ 0.0f };
		Vector3 m_obstacleHitPos{ 0.0f };
		Vector3 m_obstacleHitNrm{ 0.0f };

		bool IsValid() const { return m_pObstacle != nullptr; }
	};

	class IPerceptionProcessor
	{
	public:
		virtual PerceptedObstacleInfo& GetCurObstacleInfo() = 0;
	};
}