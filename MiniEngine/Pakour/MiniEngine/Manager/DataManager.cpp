#include "pch.h"
#include "Manager/DataManager.h"
#include "Manager/PathManager.h"
#include <fstream>
#include <nlohmann/json.hpp>
#include "Core/Log.h"

#include <cstdlib>

using json = nlohmann::json;

namespace MiniEngine 
{
	DataManager::DataManager() {}
	DataManager::~DataManager() {}

	void DataManager::Test()
	{
		std::wstring path = PathManager::GetInstance()->ResolveDataPath(L"test.json");
		std::ifstream f(path);

		if (f.is_open())
		{
			json data = json::parse(f);

			std::string str = data.dump();
			MG_LOG_INFO("Json Load Test: {}", str.c_str());
		}
	}
}