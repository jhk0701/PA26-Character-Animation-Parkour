#pragma once
#include <unordered_map>

namespace MiniEngine 
{
	class DataAsset;
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

		std::weak_ptr<DataAsset> GetData(const std::wstring& _dataAssetName);

	private:
		std::unordered_map<std::wstring, EDataFormat> m_mapFormat = { {L".json", EDataFormat::Json } };

		// data asset name - data asset 맵핑
		// 이미 로드한 거라면 다시 로드할 필요 없이 바로 쓸 것
		std::unordered_map<std::wstring, std::shared_ptr<DataAsset>> m_loadedDatas;
		
		// 데이터 에셋에서 맵핑한 클래스들의 id
		std::unordered_map<std::string, uint32_t> m_mapClassType;
	};
}