#include "pch.h"
#include "Scene/CameraComponent.h"

namespace MiniEngine
{
    Matrix CameraComponent::GetViewMatrix() const
    {
        const Matrix world = GetWorldMatrix();

        const Vector3 eye = world.Translation();
        // SimpleMath Matrix::Forward() = (-_31,-_32,-_33) = -Z basis = RH 카메라의 정면 방향.
        // (헤더 확인 완료 — 별도 부호 반전 불필요.)
        const Vector3 forward = world.Forward();
        const Vector3 up      = world.Up();

        return Matrix::CreateLookAt(eye, eye + forward, up);
    }

    Matrix CameraComponent::GetProjectionMatrix() const
    {
        return Matrix::CreatePerspectiveFieldOfView(fovYRadians, aspect, nearZ, farZ);
    }
}
