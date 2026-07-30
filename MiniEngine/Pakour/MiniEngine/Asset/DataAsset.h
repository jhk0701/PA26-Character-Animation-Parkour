#pragma once
#include <nlohmann/json.hpp>
using json = nlohmann::json;

namespace MiniEngine
{
	class RawData 
	{
	public:
		RawData(const json& _data) : m_rawData(_data) {};
		const json& GetRawData() const { return m_rawData; }

	private:
		json m_rawData;
	};

	class DataAsset
	{
	public:
		virtual ~DataAsset() {};
		virtual void Load(const json& _data) = 0;
	};
}