#pragma once
#include <memory>
#include <vector>
#include "Scene/SceneComponent.h"
#include "Asset/SkinnedMesh.h"

namespace MiniEngine
{
    class Animator;

    // 스키닝 메시 + 애니메이션 재생 상태를 보유하는 SceneComponent.
    // Tick 에서 활성 클립을 샘플해 본 최종 행렬을 갱신하고
    // 클립 전환은 PlayClip(fadeSec>0)으로 크로스페이드: 두 클립 포즈를 TRS 단계에서
    // 가중 보간(pos/scale Lerp, rot Slerp)한다.
    class SkeletalMeshComponent : public SceneComponent
    {
    public:
        SkeletalMeshComponent();

        // 활성 클립 샘플 → 본 최종 행렬(inverseBindPose * global) 갱신.
        void Tick(float _dt) override;
        void Render(Graphics::RenderContext& _context) override;

        void SetMesh(const std::shared_ptr<SkinnedMesh>& _mesh);
        std::weak_ptr<SkinnedMesh> GetMesh() const { return m_mesh; }
        std::weak_ptr<Animator> GetAnim() const { return m_anim; }

        // 렌더러가 b2 에 업로드할 본 최종 행렬 (본 개수만큼).
        std::vector<Matrix>* GetBoneMatricesPtr() { return &m_boneMatrices; }

    private:
        
        std::shared_ptr<SkinnedMesh> m_mesh;    // 스킨 메시 소유
        std::shared_ptr<Animator> m_anim;       // 본 애니메이션용
        std::vector<Matrix> m_boneMatrices;     // 스키닝 최종 행렬

        // TODO : Animator로 이전 후 제거
        //int   m_activeClip = -1;
        //float m_playTime = 0.0f; // 초 단위 누적

        //int   m_targetClip = -1;        // 전환 대상 클립(-1 = 바인드 포즈로 페이드)
        //float m_targetTime = 0.0f;      // 대상 클립 재생 시간(초)
        //float m_fadeDuration = 0.0f;    // 0 = 페이드 없음
        //float m_fadeElapsed = 0.0f;

        //LocalPoseTRS m_poseA;               // 샘플 스크래치(재할당 방지)
        //LocalPoseTRS m_poseB;
        //std::vector<Matrix> m_localPose;    // 합성 로컬 행렬 스크래치
    };
}
