#pragma once
#include <windows.h>

namespace MiniEngine
{
    // QueryPerformanceCounter 기반 고해상도 프레임 타이머.
    // 매 프레임 Tick()으로 가변 delta time(초)을 얻는다.
    class Timer
    {
    public:
        Timer();

        // 타이머 기준점을 현재 시각으로 재설정. (일시정지 해제 등)
        void Reset();

        // 직전 Tick 이후 경과 시간(초)을 반환하고 내부 기준을 갱신.
        // 디버거 정지/스파이크 대비 상한(MAX_DELTA)으로 클램프.
        float Tick();

        // Reset 이후 누적 경과 시간(초).
        float TotalTime() const;

    private:
        static constexpr float MAX_DELTA = 0.25f;

        LONGLONG m_frequency; // 초당 카운트
        LONGLONG m_startCount; // Reset 시점 카운트
        LONGLONG m_lastCount;  // 직전 Tick 시점 카운트
        float    m_deltaTime;  // 마지막 Tick의 dt(초)
    };
}
