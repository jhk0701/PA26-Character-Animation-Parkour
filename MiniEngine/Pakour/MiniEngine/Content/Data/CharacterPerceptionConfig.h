#pragma once
#include "Asset/DataAsset.h"
#include "Content/Data/PerceptionQueryTree.h"

class CharacterPerceptionConfig : public MiniEngine::DataAsset
{
public:
	void Load(const json& _data) override;
	PerceptionConfig Config;
};