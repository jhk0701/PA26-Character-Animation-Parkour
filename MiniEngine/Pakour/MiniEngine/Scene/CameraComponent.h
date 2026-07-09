#pragma once
#include "Scene/SceneComponent.h"

namespace MiniEngine
{
    // 카메라. 
    // SceneComponent의 월드 트랜스폼으로 View 행렬을 원근 파라미터로 Projection 행렬을 생성. 
    // + 왼손 좌표계 변환
    class CameraComponent : public SceneComponent
    {
    public:
        CameraComponent();

        float fovYRadians = 3.14159265f / 3.0f; // 60도
        float aspect      = 16.0f / 9.0f;
        float nearZ       = 0.1f;
        float farZ        = 1000.0f;

        // 월드 행렬에서 eye/forward/up 을 뽑아 LH LookAt 생성
        Matrix GetViewMatrix() const;
        Matrix GetProjectionMatrix() const;

        void RegisterMainCamera();
    };
}
