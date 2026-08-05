#pragma once
#include "Asset/DataAsset.h"

struct PerceptionConfig
{
	// 장애물 탐지 거리
	float minObstacleDetectDist = 1.0f;
	float maxObstacleDetectDist = 2.5f;

	// 일반 장애물 측량 파라미터
	float heightRadius = 0.5f;	// heightStep과 함께 바꿀 것
	float heightStep = 1.0f;	// = heightRadius * 2 여야 밴드가 틈이 없음
	uint8_t maxHeightStep = 3;	// 높이 측정 횟수
	float heightSearchtDist = 0.1f;

	float depthStep = 0.5f; // 확인하는 depth 깊이 단위 0.5m
	uint8_t maxDepthStep = 2;	// 최대 1.0m 까지만 잰다
	float depthSearchDownDist = 2.0f;
	float depthLift = 0.05f; // 꼭대기 표면인 경우 방지를 위해 띄어두는 크기
	
	// 매달린 상태(Hanging)의 주변 탐색 파라미터
	float onHangingSearchVDist = 1.5f;	// 상하좌우 공통 탐지 거리
	float onHangingSearchHDist = 1.5f;	// 상하좌우 공통 탐지 거리
	float onHangingSearchRadius = 0.5f;	// 상/좌/우 스피어 프로브 반지름 (하방은 단순 레이)

	// Landing 상태에서 실제로 떨어지는지 최종 확인용
	float onLandingFallingCheckDist = 0.2f;
};

class CharacterPerceptionConfig : public MiniEngine::DataAsset
{
public:
	void Load(const json& _data) override;
	PerceptionConfig Config;
};