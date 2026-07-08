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

        void Tick(float _dt) override; // 애니메이터를 통해 본 최종 행렬(inverseBindPose * global) 갱신
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
    };
}
