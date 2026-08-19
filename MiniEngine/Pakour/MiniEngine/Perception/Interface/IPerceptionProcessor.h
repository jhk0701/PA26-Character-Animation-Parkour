#pragma once
#include "Core/Math.h"

namespace MiniEngine
{
	class IObstacle;
	struct Transform;

	struct PerceptionConfig
	{
		// 장애물 탐지 거리
		float minObstacleDetectDist{ 1.0f };
		float maxObstacleDetectDist{ 2.5f };

		// 일반 장애물 측량 파라미터
		float heightRadius{ 0.5f };		// heightStep과 함께 바꿀 것
		float heightStep{ 1.0f };		// = heightRadius * 2 여야 밴드가 틈이 없음
		float heightSearchtDist{ 0.1f };
		float heightLift{ 0.05f };		// 높이 측정 시, 살짝 들어올려서 시작

		float depthStep{ 0.5f };			// 확인하는 depth 깊이 단위 0.5m
		float depthSearchDownDist{ 2.0f };
		float depthLift{ 0.05f };		// 꼭대기 표면인 경우 방지를 위해 띄어두는 크기

		float ledgeDetectRadius{ 0.25f };

		uint8_t maxHeightStep{ 3 };		// 높이 측정 횟수
		uint8_t maxDepthStep{ 2 };		// 깊이 측정 횟수
	};
	
	struct PerceptResult
	{
		IObstacle* pObstacle{ nullptr };	// 장애물 객체의 포인터 8
		Vector3 obstacleHitPos;				// 접촉 위치 12
		Vector3 obstacleHitNrm;				// 접촉 표면 노멀 벡터 12
		float obstacleDistance{ 0.0f };		// 캐릭터와 거리 4
		float obstacleLedge{ 0.0f };		// 모서리 (최종 높이) 4
		float obstacleDepth{ 0.0f };		// 깊이 4
		float roomHeight{ 0.0f };			// 여유 공간
		bool bDetectLedge{ false };			// 모서리 탐지 여부 1

		void Reset() 
		{
			pObstacle = nullptr;
			obstacleHitPos = Vector3(0.0f);
			obstacleHitNrm = Vector3(0.0f);
			obstacleDistance = 0.0f;
			obstacleLedge = 0.0f;
			obstacleDepth = 0.0f;
			roomHeight = FLT_MAX;
			bDetectLedge = false;
		}
	};

	struct PerceptedObstacleInfo
	{
		bool m_bIsNewObstacle{ true };
		IObstacle* m_pObstacle{ nullptr };

		// Travel Result와 중복 -> 정리 필요
		bool m_bDetectLedge{ false };
		float m_obstacleDistance{ 0.0f };
		float m_obstacleLedge{ 0.0f }; // 모서리 위치
		float m_obstacleDepth{ 0.0f };
		float m_obstacleHeight{ 0.0f }; // 모서리 위치 - 현재 y 위치
		float m_obstacleRoom{ 0.0f };
		Vector3 m_obstacleHitPos{ 0.0f };
		Vector3 m_obstacleHitNrm{ 0.0f };

		bool IsValid() const { return m_pObstacle != nullptr; }
	};

	class IPerceptionProcessor
	{
	public:
		virtual const PerceptionConfig& GetPerceptionConfig() const = 0;
		virtual PerceptedObstacleInfo& GetCurObstacleInfo() = 0;
		virtual IObstacle* GetCurObstacle() const = 0;
		virtual const Transform& GetTransform() const = 0;
	};
}