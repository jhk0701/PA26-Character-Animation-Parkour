#include "pch.h"
#include "Scene/CameraComponent.h"
#include "Manager/SceneManager.h"
#include "World.h"

namespace MiniEngine
{
    Matrix CameraComponent::GetViewMatrix() const
    {
        const Matrix world = GetWorldMatrix();

        const Vector3 eye = world.Translation();
        const Vector3 forward = world.Backward(); // LH 좌표계 변환
        const Vector3 up = world.Up();

        return Matrix(DirectX::XMMatrixLookAtLH(eye, eye + forward, up));
    }

    Matrix CameraComponent::GetProjectionMatrix() const
    {
        return Matrix(DirectX::XMMatrixPerspectiveFovLH(fovYRadians, aspect, nearZ, farZ)); // Matrix::CreatePerspectiveFieldOfView(fovYRadians, aspect, nearZ, farZ);
    }

    void CameraComponent::RegisterMainCamera()
    {
        std::weak_ptr<World> pWorld = SceneManager::GetInstance()->GetCurrentScene();
        if (pWorld.expired())
            return;

        pWorld.lock()->SetMainCamera(std::dynamic_pointer_cast<CameraComponent>(shared_from_this()));
    }
}
