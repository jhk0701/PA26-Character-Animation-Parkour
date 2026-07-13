#pragma once
#include <windows.h>
#include <d3dcompiler.h>
#include <wrl/client.h>
#include <string>

// 셰이더 런타임 컴파일 + exe 기준 경로 해석. Renderer/DebugDraw 가 공유한다(헤더 온리).
namespace MiniEngine::ShaderUtil
{
    // 셰이더 파일을 런타임 컴파일. 실패 시 에러 메시지를 디버그 출력.
    inline HRESULT CompileFromFile(const std::wstring& _path, const char* _entry, const char* _target, ID3DBlob** _outBlob)
    {
        UINT flags = 0;
#if defined(_DEBUG) || defined(WITH_EDITOR)
        flags |= D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
#endif
        Microsoft::WRL::ComPtr<ID3DBlob> errorBlob;
        HRESULT hr = D3DCompileFromFile(_path.c_str(), nullptr, nullptr, _entry, _target, flags, 0, _outBlob, &errorBlob);
        if (FAILED(hr) && errorBlob)
            OutputDebugStringA(static_cast<const char*>(errorBlob->GetBufferPointer()));
        return hr;
    }

    // 실행 파일 위치 기준 디렉터리(작업 디렉터리 무관).
    inline std::wstring ExeDir()
    {
        wchar_t exePath[MAX_PATH] = {};
        GetModuleFileNameW(nullptr, exePath, MAX_PATH);
        std::wstring dir(exePath);
        const size_t slash = dir.find_last_of(L"\\/");
        if (slash != std::wstring::npos)
            dir.resize(slash);
        return dir;
    }

    // exe 기준 Shaders\<name> 절대 경로.
    inline std::wstring ResolvePath(const wchar_t* _fileName)
    {
        return ExeDir() + L"\\Shaders\\" + _fileName;
    }
}
