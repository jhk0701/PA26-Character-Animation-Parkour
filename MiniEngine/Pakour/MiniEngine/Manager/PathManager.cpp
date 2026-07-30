#include "pch.h"
#include "Manager/PathManager.h"

using namespace MiniEngine;

PathManager::PathManager() {};
PathManager::~PathManager() {};

void PathManager::Init()
{
    // Assets 폴더 경로 검증 // 이미 있으면 무시됨
    CreateDirectoryW((ExeDir() + L"\\Assets").c_str(), nullptr);
    CreateDirectoryW((ExeDir() + L"\\Datas").c_str(), nullptr);
    CreateDirectoryW((ExeDir() + L"\\Shaders").c_str(), nullptr);
}

// 실행 파일 위치 기준 디렉터리(작업 디렉터리 무관).
std::wstring PathManager::ExeDir()
{
    wchar_t exePath[MAX_PATH] = {};
    GetModuleFileNameW(nullptr, exePath, MAX_PATH);
    std::wstring dir(exePath);
    const size_t slash = dir.find_last_of(L"\\/");
    if (slash != std::wstring::npos)
        dir.resize(slash);
    return dir;
}

std::wstring PathManager::GetAssetPath()
{
    return ExeDir() + L"\\Assets\\";
}

std::wstring PathManager::GetDataPath()
{
    return ExeDir() + L"\\Datas\\";
}

// exe 기준 Shaders\<name> 절대 경로.
std::wstring PathManager::ResolveShaderPath(const wchar_t* _fileName)
{
    return ExeDir() + L"\\Shaders\\" + _fileName;
}

// exe 기준 Assets\<name> 절대 경로.
std::wstring PathManager::ResolveAssetPath(const wchar_t* _fileName)
{
    return ExeDir() + L"\\Assets\\" + _fileName;
}

// exe 기준 Datas\<name> 절대 경로.
std::wstring PathManager::ResolveDataPath(const wchar_t* _fileName)
{
    return ExeDir() + L"\\Datas\\" + _fileName;
}

