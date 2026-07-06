#pragma once
#include "Core/Math.h"

namespace MiniEngine
{
    class Input;
    class CameraComponent;

    // 카메라 조작 헬퍼(컴포넌트 아님 — GameCore가 소유해 Input 접근을 깔끔히 유지).
    // Orbit: 우클릭 드래그로 공전, 휠로 거리. FPS: 우클릭 홀드+마우스로 시야 회전 + WASD/QE 이동.
    // Tab 으로 모드 전환. 매 프레임 카메라 Actor의 트랜스폼(위치/회전)을 갱신한다.
    class CameraController
    {
    public:
        enum class Mode { Orbit, Fps };

        // 초기 상태: target 을 중심으로 distance 만큼 떨어져 바라봄.
        void Initialize(const Vector3& _target, float _distance);

        // dt, 입력, 조작 대상 카메라. 카메라의 localTransform 을 직접 갱신한다.
        void Update(float _dt, const Input& _input, CameraComponent& _camera);

        Mode GetMode() const { return m_mode; }

    private:
        // yaw/pitch 로부터 정면 방향(RH, -Z 기준) 계산.
        Vector3 ForwardFromAngles() const;

        Mode m_mode = Mode::Orbit;

        // 공유 시야각.
        float m_yaw   = 0.0f; // Y축 회전(rad)
        float m_pitch = 0.0f; // X축 회전(rad)

        // Orbit 상태.
        Vector3 m_target{ 0.0f, 0.0f, 0.0f };
        float   m_distance = 5.0f;

        // FPS 상태.
        Vector3 m_fpsPosition = Vector3(0.0f, 0.0f, 5.0f);

        // 입력 델타 계산용 이전 값.
        bool m_hasPrev    = false;
        int  m_prevMouseX = 0;
        int  m_prevMouseY = 0;
        int  m_prevScroll = 0;
    };
}
