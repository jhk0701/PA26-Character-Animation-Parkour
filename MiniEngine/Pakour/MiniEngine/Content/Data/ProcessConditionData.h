#pragma once
#include "Asset/DataAsset.h"

namespace MiniEngine 
{
	class ProcessData;
}

class ProcessConditionData : public MiniEngine::DataAsset
{
public:
	void Load(const json& _data) override;
	void ConstructData(std::vector<std::shared_ptr<MiniEngine::ProcessData>>& _out);

private:
	
};
