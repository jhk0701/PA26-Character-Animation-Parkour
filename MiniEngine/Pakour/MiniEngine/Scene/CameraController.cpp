#include "pch.h"
#include "Scene/CameraController.h"
#include "Scene/CameraComponent.h"
#include "Platform/Input.h"
#include "Core/Log.h"

namespace MiniEngine
{
    namespace
    {
        constexpr float ROTATE_SENSITIVITY = 0.005f;  // rad / 픽셀
        constexpr float ZOOM_SENSITIVITY   = 0.0015f;  // 거리 / 휠틱
        constexpr float MOVE_SPEED         = 5.0f;      // 유닛 / 초
        constexpr float PITCH_LIMIT        = 1.55334f;  // ~89도
        constexpr float MIN_DISTANCE       = 0.5f;
    }

    void CameraController::Initialize(const Vector3& _target, float _distance)
    {
        m_target = _target;
        m_distance = _distance;
        m_yaw = 0.0f;
        m_pitch = 0.0f;
    
        // 초기 FPS 위치는 orbit eye 와 일치시킨다.
        m_fpsPosition = m_target - ForwardFromAngles() * m_distance;
    }

    Vector3 CameraController::ForwardFromAngles() const
    {
        // 정면 방향을 카메라 회전 쿼터니언에서 직접 파생 → GetViewMatrix가 쓰는 회전과
        const Quaternion rot = Quaternion::CreateFromYawPitchRoll(m_yaw, m_pitch, 0.0f);
        return Vector3::Transform(Vector3(0.0f, 0.0f, 1.0f), rot); // +Z = LH 정면
    }

    void CameraController::Update(float _dt, const Input& _input, CameraComponent& _camera)
    {
        // ----- 입력 델타 -----
        const int mouseX = _input.MouseX();
        const int mouseY = _input.MouseY();
        const int scroll = _input.ScrollValue();
        if (!m_hasPrev)
        {
            m_prevMouseX = mouseX;
            m_prevMouseY = mouseY;
            m_prevScroll = scroll;
            m_hasPrev = true;
        }
        const int dx = mouseX - m_prevMouseX;
        const int dy = mouseY - m_prevMouseY;
        const int dScroll = scroll - m_prevScroll;
        m_prevMouseX = mouseX;
        m_prevMouseY = mouseY;
        m_prevScroll = scroll;

        // ----- 모드 토글 (Tab) -----
        if (_input.IsKeyPressed(Input::Keys::Tab))
        {
            if (m_mode == Mode::Orbit)
            {
                // orbit → fps: 현재 orbit eye 를 fps 위치로 승계.
                m_fpsPosition = m_target - ForwardFromAngles() * m_distance;
                m_mode = Mode::Fps;
                MG_LOG_INFO("Camera mode: FPS");
            }
            else
            {
                // fps → orbit: 정면 distance 앞에 target 배치.
                m_target = m_fpsPosition + ForwardFromAngles() * m_distance;
                m_mode = Mode::Orbit;
                MG_LOG_INFO("Camera mode: Orbit");
            }
        }

        // ----- 회전 (우클릭 홀드 중) -----
        if (_input.RightDown())
        {
            m_yaw   += dx * ROTATE_SENSITIVITY;
            m_pitch -= dy * ROTATE_SENSITIVITY; // 화면 위로 드래그 = 위를 봄
            m_pitch = std::clamp(m_pitch, -PITCH_LIMIT, PITCH_LIMIT);
        }

        Vector3 eye;
        const Vector3 forward = ForwardFromAngles();

        if (m_mode == Mode::Orbit)
        {
            // 휠로 거리 조절.
            if (dScroll != 0)
            {
                m_distance -= dScroll * ZOOM_SENSITIVITY;
                if (m_distance < MIN_DISTANCE)
                    m_distance = MIN_DISTANCE;
            }
            eye = m_target - forward * m_distance;
        }
        else // Fps
        {
            // WASD/QE 이동 (forward/right/up 기준).
            const Vector3 worldUp(0.0f, 1.0f, 0.0f);
            Vector3 right = worldUp.Cross(forward);
            right.Normalize();

            Vector3 move(0.0f, 0.0f, 0.0f);
            using Keys = Input::Keys;
            if (_input.IsKeyDown(Keys::W)) move += forward;
            if (_input.IsKeyDown(Keys::S)) move -= forward;
            if (_input.IsKeyDown(Keys::D)) move += right;
            if (_input.IsKeyDown(Keys::A)) move -= right;
            if (_input.IsKeyDown(Keys::E)) move += worldUp;
            if (_input.IsKeyDown(Keys::Q)) move -= worldUp;

            if (move.LengthSquared() > 0.0f)
            {
                move.Normalize();
                m_fpsPosition += move * (MOVE_SPEED * _dt);
            }
            eye = m_fpsPosition;
        }

        // ----- 카메라 트랜스폼 갱신 -----
        _camera.localTransform.position = eye;
        _camera.localTransform.rotation = Quaternion::CreateFromYawPitchRoll(m_yaw, m_pitch, 0.0f);
    }
}
