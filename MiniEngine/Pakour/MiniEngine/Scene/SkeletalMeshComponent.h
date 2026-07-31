#pragma once
#include <memory>
#include <vector>
#include "Scene/SceneComponent.h"
#include "Asset/SkinnedMesh.h"
#include "Asset/RootMotion.h"

namespace MiniEngine
{
    class Animator;
    struct IKGoal;
    struct TwoBoneIKBinding;

    // 스키닝 메시 + 애니메이션 재생 상태를 보유하는 SceneComponent.
    // Tick 에서 활성 클립을 샘플해 본 최종 행렬을 갱신하고
    // 클립 전환은 PlayClip(fadeSec>0)으로 크로스페이드: 두 클립 포즈를 TRS 단계에서
    // 가중 보간(pos/scale Lerp, rot Slerp)한다.
    class SkeletalMeshComponent : public SceneComponent
    {
    public:
        void Tick(float _dt) override; // 애니메이터를 통해 본 최종 행렬(inverseBindPose * global) 갱신
        void LateTick(float _dt) override;

        void Render(Graphics::RenderContext& _context) override;

        void SetMesh(const std::shared_ptr<SkinnedMesh>& _mesh);
        void SetColor(const Vector3& _col) { m_color = _col; }

        std::weak_ptr<SkinnedMesh> GetMesh() const { return m_mesh; }
        std::weak_ptr<Animator> GetAnim() const { return m_anim; }

        // 렌더러가 b2 에 업로드할 본 최종 행렬 (본 개수만큼).
        std::vector<Matrix>* GetBoneMatricesPtr() { return &m_boneMatrices; }
        
        void SampleAnimation(float _dt);

        RootMotionDelta ConsumeRootMotionDelta();
        bool IsRootMotionEnabled() const;

        // IK 관련
        // 월드 스페이스 Goal 좌표 입력
        void SetIKGoalWorld(const TwoBoneIKBinding& _boneBinding, const Vector3& _worldPos, const Quaternion& _worldRot, float _posAlpha, float _rotAlpha);

        // 위치만 입력하는 오버로드
        // 회전은 애니메이션대로 처리
        void SetIKGoalWorld(const TwoBoneIKBinding& _boneBinding, const Vector3& _worldPos, float _posAlpha);

        // 회전을 현재 포즈에 곱할 월드 델타로 지정 (지면 법선 정렬 등)
        void SetIKGoalWorldWithRotDelta(const TwoBoneIKBinding& _boneBinding, const Vector3& _worldPos, float _posAlpha,
                                        const Quaternion& _worldRotDelta, float _rotAlpha);

        // 2본 ik에서 pole(중간 관절)이 향할 지점을 지정
        // 없으면 현재 애니메이션의 포즈의 굽힘 평면을 그대로 유지 -> 굳이 호출할 필요는 없음
        void SetIKPoleTargetWorld(const TwoBoneIKBinding& _boneBinding, const Vector3& _worldPole);

        // 관절 한계 (각도, degree). 0 / 180 / 180 이면 무제한 = 기존 동작.
        //  _minBendDeg   : lower 관절(무릎/팔꿈치) 내각 하한 — 과도한 접힘 방지
        //  _maxBendDeg   : 같은 내각 상한 — 과신전 방지.
        //                  ⚠ 기존 사거리 상한이 이미 약 168도라, 그보다 낮춰야 효과가 있다.
        //  _swingConeDeg : upper(허벅지/상완)가 rest 방향에서 벌어질 최대각.
        //                  걸면 end 가 타깃에 의도적으로 미달한다.
        void SetIKLimits(const TwoBoneIKBinding& _boneBinding,
                         float _minBendDeg, float _maxBendDeg, float _swingConeDeg);

        void ClearIKGoal(HumanoidBone _end);
        void ClearAllIKGoal();
        const IKGoal* GetIKGoal(HumanoidBone _end) const;

        // 골반 하강용 오프셋 -> 발 IK 시, 몸 낮추기용
        void SetIKPelvisOffsetWorld(const Vector3& _worldOffset);
        
        bool GetBoneWorldMatrix(int _boneIdx, Matrix& _out) const;

        void SolveIK();

    private:
        Vector3 m_color{ 0.5f, 0.7f, 0.5f };
        std::shared_ptr<SkinnedMesh> m_mesh;    // 스킨 메시 소유

        std::shared_ptr<Animator> m_anim;       // 본 애니메이션용
        std::vector<Matrix> m_boneMatrices;     // 스키닝 최종 행렬

    };
}
