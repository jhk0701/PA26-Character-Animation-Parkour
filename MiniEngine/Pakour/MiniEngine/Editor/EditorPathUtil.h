#pragma once
// 에디터 패널 공용 경로 헬퍼 — ImGui InputText(UTF-8 char 버퍼)와 엔진 API(std::wstring) 사이 변환.
// 헤더 온리(inline). Assimp/ImGui 를 참조하지 않으므로 전 구성에서 안전하다.
#include <windows.h>
#include <string>
#include <vector>
#include <algorithm>
#include <cwctype>
#include <filesystem>
#include <system_error>

namespace MiniEngine
{
    namespace Editor
    {
        // UTF-8 char 버퍼(ImGui InputText) → wstring 경로.
        inline std::wstring ToWide(const char* _utf8)
        {
            if (!_utf8 || !*_utf8) return {};
            const int len = ::MultiByteToWideChar(CP_UTF8, 0, _utf8, -1, nullptr, 0);
            if (len <= 0) return {};
            std::wstring out(static_cast<size_t>(len - 1), L'\0'); // len 은 널 종단 포함
            ::MultiByteToWideChar(CP_UTF8, 0, _utf8, -1, out.data(), len);
            return out;
        }

        // wstring → UTF-8 std::string (표시/버퍼 초기화용).
        inline std::string ToUtf8(const std::wstring& _wide)
        {
            if (_wide.empty()) return {};
            const int len = ::WideCharToMultiByte(CP_UTF8, 0, _wide.c_str(), -1, nullptr, 0, nullptr, nullptr);
            if (len <= 0) return {};
            std::string out(static_cast<size_t>(len - 1), '\0');
            ::WideCharToMultiByte(CP_UTF8, 0, _wide.c_str(), -1, out.data(), len, nullptr, nullptr);
            return out;
        }

        // ';' 구분 다중 경로 입력 → wstring 목록(앞뒤 공백 트림, 빈 항목 스킵).
        inline std::vector<std::wstring> SplitPaths(const char* _utf8)
        {
            std::vector<std::wstring> out;
            if (!_utf8) return out;
            std::string buf(_utf8);
            size_t begin = 0;
            while (begin <= buf.size())
            {
                size_t end = buf.find(';', begin);
                if (end == std::string::npos) end = buf.size();
                std::string item = buf.substr(begin, end - begin);
                const size_t first = item.find_first_not_of(" \t");
                const size_t last  = item.find_last_not_of(" \t");
                if (first != std::string::npos)
                {
                    item = item.substr(first, last - first + 1);
                    out.push_back(ToWide(item.c_str()));
                }
                begin = end + 1;
            }
            return out;
        }

        // 폴더 안의 애니 소스를 전부 수집(비재귀). 소스 모델 자기 자신은 제외.
        // 없는 경로/파일이면 조용히 빈 목록. 결정적 클립 순서를 위해 정렬한다.
        inline std::vector<std::wstring> EnumerateAnimFolder(const char* _utf8Dir,
                                                            const std::wstring& _srcToSkip)
        {
            std::vector<std::wstring> out;
            if (!_utf8Dir || !*_utf8Dir) return out;
            std::error_code ec;
            const std::filesystem::path dir(ToWide(_utf8Dir));
            if (!std::filesystem::is_directory(dir, ec)) return out;
            static const wchar_t* kExts[] = { L".fbx", L".gltf", L".glb", L".dae" }; // .obj 는 애니 없음
            std::error_code srcEc;
            for (std::filesystem::directory_iterator it(dir, ec), end; it != end; it.increment(ec))
            {
                if (ec) break;
                if (!it->is_regular_file(ec)) continue;
                std::wstring ext = it->path().extension().wstring();
                std::transform(ext.begin(), ext.end(), ext.begin(),
                               [](wchar_t _c) { return static_cast<wchar_t>(::towlower(_c)); });
                bool ok = false;
                for (const wchar_t* x : kExts) if (ext == x) { ok = true; break; }
                if (!ok) continue;
                const std::filesystem::path full = it->path();
                if (!_srcToSkip.empty() &&
                    std::filesystem::equivalent(full, std::filesystem::path(_srcToSkip), srcEc))
                    continue; // 소스 모델 자기 자신 제외(경로 표기 차이도 흡수)
                out.push_back(full.wstring());
            }
            std::sort(out.begin(), out.end());
            return out;
        }
    }
}
