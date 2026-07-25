#pragma once
#include "Core/Math.h"

namespace MiniEngine
{
    // 위치/회전/스케일을 갖는 트랜스폼. SimpleMath(row-major, row-vector) 기반.
    // 로컬 행렬 = Scale * Rotation * Translation (row-vector 관례: v' = v * M).
    struct Transform
    {
        // SimpleMath 정적 상수(Vector3::Zero 등)는 DirectXTK 라이브러리 .cpp 정의에
        // 링크 의존하므로, 헤더-인라인 자립을 위해 명시적 리터럴로 초기화한다.
        Vector3    position{ 0.0f, 0.0f, 0.0f };
        Quaternion rotation{ 0.0f, 0.0f, 0.0f, 1.0f }; // identity
        Vector3    scale{ 1.0f, 1.0f, 1.0f };

        Matrix GetMatrix() const
        {
            return Matrix::CreateScale(scale)
                 * Matrix::CreateFromQuaternion(rotation)
                 * Matrix::CreateTranslation(position);
        }

        inline Vector3 Forward() const { return GetMatrix().Forward(); }
        inline Vector3 Right() const { return -GetMatrix().Right(); }
        inline Vector3 Up() const { return GetMatrix().Up(); }
    };
}
