#include "pch.h"
#include "Manager/DataManager.h"

#include <fstream>
#include <filesystem>
#include <nlohmann/json.hpp>

#include "Manager/PathManager.h"
#include "Core/Log.h"

#include <cstdlib>

using json = nlohmann::json;
namespace fs = std::filesystem;

namespace MiniEngine 
{
	DataManager::DataManager() {}
	DataManager::~DataManager() {}

	void DataManager::Init()
	{
		LoadAllDataAsset();
	}

	void DataManager::LoadAllDataAsset()
	{
		// 일반적으로 Datas 경로는 PathManager가 앞에서 이미 검증했을 것
		const std::wstring DATA_PATH = PathManager::GetInstance()->GetDataPath();
		if (!fs::exists(DATA_PATH) || !fs::is_directory(DATA_PATH))
		{
			MG_LOG_ERROR("[DataManager::LoadAllDataAsset] Path is not found.");
			return;
		}

		try 
		{
			for (const auto& entry : fs::directory_iterator(DATA_PATH))
			{
				const fs::path& p = entry.path();
				if (m_mapFormat[p.extension().wstring()] == EDataFormat::None)
					continue;

				LoadDataAsset(p.wstring(), p.filename().wstring());
			}
		}
		catch (const fs::filesystem_error& e) 
		{
			MG_LOG_ERROR("[DataManager::LoadAllDataAsset] Error occured in reading files. {}", e.what());
		}
	}

	void DataManager::LoadDataAsset(const std::wstring& _path, const std::wstring& _name)
	{
		// 이미 로드된 정보가 있다면 스킵
		if (m_loadedRawDatas[_name] != nullptr)
			return;

		std::ifstream f(_path);

		if (f.is_open())
		{
			json rawData = json::parse(f);
			m_loadedRawDatas[_name] = std::make_shared<RawData>(rawData);
		}
		else 
			MG_LOG_INFO("[DataManager::LoadDataAsset] can't open file");

		f.close();
	}

	bool DataManager::TryGetRawData(const std::wstring& _inName, std::shared_ptr<RawData>& _outData)
	{
		auto it = m_loadedRawDatas.find(_inName);
		if (it == m_loadedRawDatas.end())
			return false;

		_outData = it->second;
		return true;
	};
}