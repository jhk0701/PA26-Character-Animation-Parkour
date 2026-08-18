#pragma once
#include "Asset/DataAsset.h"
#include "Perception/Interface/IPerceptionProcessor.h"

struct CharacterConfig : public MiniEngine::PerceptionConfig
{
	// Landing 상태에서 실제로 떨어지는지 최종 확인용
	float onLandingFallingCheckDist = 0.2f;
};

class CharacterConfigData : public MiniEngine::DataAsset
{
public:
	void Load(const json& _data) override;
	CharacterConfig Config;
};