#include "pch.h"
#include "Scene/SkeletalMeshComponent.h"

namespace MiniEngine
{
    void SkeletalMeshComponent::SetMesh(const std::shared_ptr<SkinnedMesh>& _mesh)
    {
        m_mesh = _mesh;
        m_playTime = 0.0f;
        // 페이드 상태도 리셋 — 이전 메시의 클립 인덱스/시간은 새 메시에 무의미.
        m_targetClip   = -1;
        m_targetTime   = 0.0f;
        m_fadeDuration = 0.0f;
        m_fadeElapsed  = 0.0f;
        RefreshBoneMatrices();
    }

    void SkeletalMeshComponent::PlayClip(int _clipIndex, float _fadeSec)
    {
        // 현재가 정지(-1)이거나 페이드 시간이 없으면 즉시 전환(기존 SetActiveClip 동작).
        if (_fadeSec <= 0.0f || m_activeClip < 0)
        {
            m_activeClip   = _clipIndex;
            m_playTime     = 0.0f;
            m_targetClip   = -1;
            m_targetTime   = 0.0f;
            m_fadeDuration = 0.0f;
            m_fadeElapsed  = 0.0f;
            RefreshBoneMatrices();
            return;
        }

        // 크로스페이드 시작(-1 대상이면 바인드 포즈로 페이드).
        // 페이드 중 재호출이면 현재 진행 포즈 기준으로 새 페이드를 시작하는 대신
        // 대상만 교체(단순화 — 시각적으로 충분히 부드러움).
        m_targetClip   = _clipIndex;
        m_targetTime   = 0.0f;
        m_fadeDuration = _fadeSec;
        m_fadeElapsed  = 0.0f;
    }

    void SkeletalMeshComponent::Tick(float _dt)
    {
        SceneComponent::Tick(_dt);

        if (!m_mesh)
            return;

        const bool playing = (m_activeClip >= 0
            && m_activeClip < static_cast<int>(m_mesh->GetClips().size()));
        if (!playing && !IsFading())
            return; // 정지 상태 — 바인드 포즈 유지(행렬은 이미 계산됨)

        m_playTime += _dt;

        if (IsFading())
        {
            m_targetTime  += _dt;
            m_fadeElapsed += _dt;
            if (m_fadeElapsed >= m_fadeDuration)
            {
                // 페이드 완료 — 대상을 현재로 승격.
                m_activeClip   = m_targetClip;
                m_playTime     = m_targetTime;
                m_targetClip   = -1;
                m_targetTime   = 0.0f;
                m_fadeDuration = 0.0f;
                m_fadeElapsed  = 0.0f;
            }
        }

        RefreshBoneMatrices();
    }

    void SkeletalMeshComponent::SamplePose(int _clipIndex, float _timeSec, LocalPoseTRS& _outPose) const
    {
        if (_clipIndex >= 0 && _clipIndex < static_cast<int>(m_mesh->GetClips().size()))
            m_mesh->GetClips()[_clipIndex].SampleTRS(_timeSec, m_mesh->GetSkeleton(), _outPose);
        else
            SampleBindPoseTRS(m_mesh->GetSkeleton(), _outPose); // 클립 없음 — 바인드 포즈
    }

    void SkeletalMeshComponent::RefreshBoneMatrices()
    {
        if (!m_mesh)
        {
            m_boneMatrices.clear();
            return;
        }

        const Skeleton& skeleton = m_mesh->GetSkeleton();

        SamplePose(m_activeClip, m_playTime, m_poseA);
        if (IsFading())
        {
            // 현재 → 대상 크로스페이드: TRS 가중 보간(pos/scale Lerp, rot Slerp). (§9)
            const float weight = m_fadeElapsed / m_fadeDuration;
            SamplePose(m_targetClip, m_targetTime, m_poseB);
            BlendPose(m_poseA, m_poseB, weight, m_poseA);
        }
        ComposePose(m_poseA, m_localPose);

        skeleton.ComputeBoneMatrices(m_localPose, m_boneMatrices);
    }
}
