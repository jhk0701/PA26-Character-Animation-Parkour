#pragma once
#include <directxtk/SimpleMath.h>

// 엔진 공용 수학 타입 별칭. DirectXTK SimpleMath(MIT) 기반.
// 이후 Scene/Camera/Animation 증분에서 이 별칭들을 사용한다.
namespace MiniEngine
{
    using Vector2    = DirectX::SimpleMath::Vector2;
    using Vector3    = DirectX::SimpleMath::Vector3;
    using Vector4    = DirectX::SimpleMath::Vector4;
    using Quaternion = DirectX::SimpleMath::Quaternion;
    using Matrix     = DirectX::SimpleMath::Matrix;
    using Color      = DirectX::SimpleMath::Color;

    constexpr float PI      = 3.14159265358979323846f;
    constexpr float TWO_PI  = 2.0f * PI;
    constexpr float HALF_PI = 0.5f * PI;

    inline float ToRadians(float _degrees) { return _degrees * (PI / 180.0f); }
    inline float ToDegrees(float _radians) { return _radians * (180.0f / PI); }
}
