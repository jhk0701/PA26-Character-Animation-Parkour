#pragma once
#include <cstdint>
#include "Core/Math.h"

namespace MiniEngine
{
    // 디버그 렌더 최소 단위. Physics 가 생산하고 Renderer(DebugDrawer)가 소비한다.
    // 두 모듈이 서로를 include 하지 않도록 중립 지대(Core)에 둔 타입이다.
    //
    // colorARGB 는 PhysX PxDebugColor 와 동일한 0xAARRGGBB 패킹을 그대로 쓴다.
    // 리틀엔디언 메모리 바이트가 (B,G,R,A) 라 DXGI_FORMAT_B8G8R8A8_UNORM 과 정확히 일치 →
    // 정점 업로드 시 색 변환 코드가 필요 없다. (Renderer/DebugDraw.cpp 입력 레이아웃)
    struct DebugLine
    {
        Vector3  a;
        Vector3  b;
        uint32_t colorARGB = 0xFFFFFFFFu;
    };

    // 자주 쓰는 색(PxDebugColor 와 동일 값).
    namespace DebugColor
    {
        constexpr uint32_t RED     = 0xFFFF0000u;
        constexpr uint32_t GREEN   = 0xFF00FF00u;
        constexpr uint32_t BLUE    = 0xFF0000FFu;
        constexpr uint32_t YELLOW  = 0xFFFFFF00u;
        constexpr uint32_t MAGENTA = 0xFFFF00FFu;
        constexpr uint32_t CYAN    = 0xFF00FFFFu;
        constexpr uint32_t WHITE   = 0xFFFFFFFFu;
    }
}
