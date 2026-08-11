#pragma once
#include "Asset/DataAsset.h"
#include "Perception/Interface/IPerceptionProcessor.h"

struct CharacterConfig : public MiniEngine::PerceptionConfig
{
	float ledgeDetectRadius = 0.5f;
	
	// 매달린 상태(Hanging)의 주변 탐색 파라미터
	float onHangingSearchDist = 1.5f;	// 상하좌우 공통 탐지 거리
	float onHangingSearchFwdDist = 0.5f;	// 벽에 매달렸을 때, 벽이 이어지는 정면을 향해 쏘는 거리 (깊이 확인)
	float onHangingSearchRadius = 0.5f;	// 상/좌/우 스피어 프로브 반지름 (하방은 단순 레이)
	float onHangingMovableRange = 1.5f; // 벽면 매달려 이동 시, 체크하는 이동 가능 범위

	// Landing 상태에서 실제로 떨어지는지 최종 확인용
	float onLandingFallingCheckDist = 0.2f;
};

class CharacterConfigData : public MiniEngine::DataAsset
{
public:
	void Load(const json& _data) override;
	CharacterConfig Config;
};