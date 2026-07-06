#pragma once

// spdlog 래퍼. Debug/Editor에서만 활성, Release(MG_RELEASE)에서는 완전 no-op.
// 사용: MG_LOG_INFO("dt = {:.3f} ms", dtMs);  등 spdlog 포맷({}) 그대로.

#if !defined(MG_RELEASE)

#include <memory>
#include <spdlog/spdlog.h>

namespace MiniEngine
{
    class Log
    {
    public:
        // 로거/싱크 초기화. main 진입 초반에 1회 호출.
        static void Init();

        static std::shared_ptr<spdlog::logger>& GetLogger() { return s_logger; }

    private:
        static std::shared_ptr<spdlog::logger> s_logger;
    };
}

#define MG_LOG_INFO(...)  ::MiniEngine::Log::GetLogger()->info(__VA_ARGS__)
#define MG_LOG_WARN(...)  ::MiniEngine::Log::GetLogger()->warn(__VA_ARGS__)
#define MG_LOG_ERROR(...) ::MiniEngine::Log::GetLogger()->error(__VA_ARGS__)

#else // MG_RELEASE

namespace MiniEngine
{
    class Log
    {
    public:
        static void Init() {} // Release: no-op
    };
}

#define MG_LOG_INFO(...)  ((void)0)
#define MG_LOG_WARN(...)  ((void)0)
#define MG_LOG_ERROR(...) ((void)0)

#endif
