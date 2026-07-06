#include "pch.h"
#include "Core/Log.h"

// Release에서는 spdlog 심볼을 전혀 참조하지 않도록 구현부 전체를 격리한다.
#if !defined(MG_RELEASE)

#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/sinks/msvc_sink.h>

namespace MiniEngine
{
    std::shared_ptr<spdlog::logger> Log::s_logger;

    void Log::Init()
    {
        if (s_logger)
            return;

        // 콘솔(색상) + MSVC 출력 창(OutputDebugString) 두 싱크로 동시 출력.
        std::vector<spdlog::sink_ptr> sinks;
        sinks.push_back(std::make_shared<spdlog::sinks::stdout_color_sink_mt>());
        sinks.push_back(std::make_shared<spdlog::sinks::msvc_sink_mt>());

        // 예: [14:22:01.123] [MiniEngine] [info] message
        for (auto& sink : sinks)
            sink->set_pattern("[%T.%e] [%n] [%^%l%$] %v");

        s_logger = std::make_shared<spdlog::logger>("MiniEngine", sinks.begin(), sinks.end());
        s_logger->set_level(spdlog::level::trace);
        s_logger->flush_on(spdlog::level::warn);
        spdlog::register_logger(s_logger);
    }
}

#endif
