#pragma once
#include "Asset/DataAsset.h"
#include <unordered_map>

namespace MiniEngine 
{
	class DataManager
	{
		SINGLETON(DataManager);
	public:
		enum EDataFormat : uint8_t 
		{
			None,
			Json,
		};

		// 게임 시작 시점에서 data asset은 미리 모두 로드할 것
		void Init();
		void LoadAllDataAsset();
		void LoadDataAsset(const std::wstring& _path, const std::wstring& _name);

		bool TryGetRawData(const std::wstring& _inName, std::shared_ptr<RawData>& _outData);
		template<typename T>
		bool TryGetDataAsset(const std::wstring& _name, std::shared_ptr<T>& _outDataAsset);

	private:
		std::unordered_map<std::wstring, EDataFormat> m_mapFormat = { {L".json", EDataFormat::Json } };

		// data asset name - data asset 맵핑
		// 이미 로드한 거라면 다시 로드할 필요 없이 바로 쓸 것
		std::unordered_map<std::wstring, std::shared_ptr<RawData>> m_loadedRawDatas;
		std::unordered_map<std::wstring, std::shared_ptr<DataAsset>> m_loadedDataAssets;
	};

	template<typename T>
	inline bool DataManager::TryGetDataAsset(const std::wstring& _name, std::shared_ptr<T>& _outDataAsset) 
	{
		if (m_loadedDataAssets.find(_name) != m_loadedDataAssets.end())
		{
			_outDataAsset = std::dynamic_pointer_cast<T>(m_loadedDataAssets[_name]);
			return true;
		}

		std::shared_ptr<RawData> pRawData;
		if (TryGetRawData(_name, pRawData) == false)
			return false;

		std::shared_ptr<T> newDataAsset = std::make_shared<T>();
		newDataAsset->Load(pRawData->GetRawData());

		m_loadedDataAssets[_name] = newDataAsset;
		_outDataAsset = newDataAsset;

		return true;
	}
}