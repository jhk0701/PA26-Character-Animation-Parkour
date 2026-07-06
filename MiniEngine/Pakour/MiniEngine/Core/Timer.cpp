#include "pch.h"
#include "Core/Timer.h"

namespace MiniEngine
{
    Timer::Timer()
        : m_frequency(1), m_startCount(0), m_lastCount(0), m_deltaTime(0.0f)
    {
        LARGE_INTEGER freq;
        QueryPerformanceFrequency(&freq);
        m_frequency = freq.QuadPart;
        Reset();
    }

    void Timer::Reset()
    {
        LARGE_INTEGER now;
        QueryPerformanceCounter(&now);
        m_startCount = now.QuadPart;
        m_lastCount = now.QuadPart;
        m_deltaTime = 0.0f;
    }

    float Timer::Tick()
    {
        LARGE_INTEGER now;
        QueryPerformanceCounter(&now);

        const LONGLONG delta = now.QuadPart - m_lastCount;
        m_lastCount = now.QuadPart;

        m_deltaTime = static_cast<float>(static_cast<double>(delta) / static_cast<double>(m_frequency));
        if (m_deltaTime < 0.0f)        // 카운터 이상치 방어
            m_deltaTime = 0.0f;
        if (m_deltaTime > MAX_DELTA)   // 디버거 정지/스파이크 클램프
            m_deltaTime = MAX_DELTA;

        return m_deltaTime;
    }

    float Timer::TotalTime() const
    {
        LARGE_INTEGER now;
        QueryPerformanceCounter(&now);
        return static_cast<float>(static_cast<double>(now.QuadPart - m_startCount) / static_cast<double>(m_frequency));
    }
}
