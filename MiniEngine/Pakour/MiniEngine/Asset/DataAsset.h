#pragma once
#include <nlohmann/json.hpp>

using json = nlohmann::json;

namespace MiniEngine
{
	class DataAsset 
	{
	public:
		virtual ~DataAsset() {};
		virtual void Init(const json& _data) {}; // 맵핑된 클래스로 만들어졌을 것. 하위 클래스에서 초기화(데이터 읽기 처리)
	};
}