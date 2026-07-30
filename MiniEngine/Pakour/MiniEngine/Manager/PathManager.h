#pragma once

namespace MiniEngine 
{
	class PathManager
	{
		SINGLETON(PathManager)

	public:
		void Init();

		std::wstring ExeDir();
		std::wstring GetAssetPath();
		std::wstring GetDataPath();

		std::wstring ResolveShaderPath(const wchar_t* _fileName);
		std::wstring ResolveAssetPath(const wchar_t* _fileName);
		std::wstring ResolveDataPath(const wchar_t* _fileName);
	};
}
