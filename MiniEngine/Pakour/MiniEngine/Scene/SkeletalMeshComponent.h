#pragma once
#include <memory>
#include <vector>
#include "Scene/SceneComponent.h"
#include "Asset/SkinnedMesh.h"

namespace MiniEngine
{
    // 스키닝 메시 + 애니메이션 재생 상태를 보유하는 SceneComponent. (CLAUDE.md §6/§9)
    // Tick 에서 활성 클립을 샘플해 본 최종 행렬을 갱신하고(§7 애니메이션 슬롯),
    // 실제 드로우는 GameCore/렌더러가 순회하며 수행(StaticMeshComponent 와 동일 분업).
    // 클립 전환은 PlayClip(fadeSec>0)으로 크로스페이드: 두 클립 포즈를 TRS 단계에서
    // 가중 보간(pos/scale Lerp, rot Slerp)한다. (§9 블렌딩)
    class SkeletalMeshComponent : public SceneComponent
    {
    public:
        void SetMesh(const std::shared_ptr<SkinnedMesh>& _mesh);
        const std::shared_ptr<SkinnedMesh>& GetMesh() const { return m_mesh; }

        // 클립 재생/전환. _clipIndex 범위 밖(-1 포함) = 정지(바인드 포즈).
        // _fadeSec>0 이고 현재 재생 중이면 크로스페이드, 아니면 즉시 전환.
        // 정지 대상(-1)으로도 페이드 가능(바인드 포즈로 블렌드).
        void PlayClip(int _clipIndex, float _fadeSec = 0.0f);

        // 재생할 클립 인덱스(-1 = 정지, 바인드 포즈 유지). 즉시 전환 = PlayClip(i, 0).
        void SetActiveClip(int _clipIndex) { PlayClip(_clipIndex, 0.0f); }
        int  GetActiveClip() const { return m_activeClip; }

        // 크로스페이드 진행 중인지(에디터 UI 표시용).
        bool IsFading() const { return m_fadeDuration > 0.0f; }

        // 활성 클립 샘플 → 본 최종 행렬(inverseBindPose * global) 갱신.
        void Tick(float _dt) override;

        // 렌더러가 b2 에 업로드할 본 최종 행렬 (본 개수만큼).
        const std::vector<Matrix>& GetBoneMatrices() const { return m_boneMatrices; }

    private:
        // 본 행렬을 현재 재생 시간(+페이드 가중치) 기준으로 다시 계산.
        void RefreshBoneMatrices();

        // _clipIndex 가 유효 클립이면 SampleTRS, 아니면 바인드 포즈 TRS.
        void SamplePose(int _clipIndex, float _timeSec, LocalPoseTRS& _outPose) const;

        std::shared_ptr<SkinnedMesh> m_mesh; // 사용처 소유(shared). (§12)

        int   m_activeClip = -1;
        float m_playTime   = 0.0f; // 초 단위 누적

        // ── 크로스페이드 상태 (fadeDuration>0 일 때만 유효) ──
        int   m_targetClip   = -1;   // 전환 대상 클립(-1 = 바인드 포즈로 페이드)
        float m_targetTime   = 0.0f; // 대상 클립 재생 시간(초)
        float m_fadeDuration = 0.0f; // 0 = 페이드 없음
        float m_fadeElapsed  = 0.0f;

        LocalPoseTRS m_poseA;               // 샘플 스크래치(재할당 방지)
        LocalPoseTRS m_poseB;
        std::vector<Matrix> m_localPose;    // 합성 로컬 행렬 스크래치
        std::vector<Matrix> m_boneMatrices; // 스키닝 최종 행렬
    };
}
