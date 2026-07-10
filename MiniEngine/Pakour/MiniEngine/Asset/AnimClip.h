#pragma once
#include <string>
#include <vector>
#include "Core/Math.h"
#include "Scene/Transform.h"
#include "Asset/Skeleton.h"

namespace MiniEngine
{
    // 키프레임 (time 단위는 tick — 초 = tick / ticksPerSecond).
    struct VecKey
    {
        float   time = 0.0f;
        Vector3 value;
    };

    struct QuatKey
    {
        float      time = 0.0f;
        Quaternion value;
    };

    // 본 1개의 키 트랙. 비어있는 트랙은 바인드 포즈 성분을 유지한다.
    struct AnimChannel
    {
        int boneIndex = -1;
        std::vector<VecKey>  pos;
        std::vector<QuatKey> rot;
        std::vector<VecKey>  scale;
    };

    // 본 1개의 로컬 포즈 Transform
    struct BoneTRS
    {
        Vector3    pos;
        Quaternion rot;
        Vector3    scale = Vector3(1.0f, 1.0f, 1.0f);
    };

    using LocalPoseTRS = std::vector<BoneTRS>;     // 스켈레톤 전체의 로컬 포즈(본 순서 = Skeleton::bones 순서).

    // 애니메이션 클립 시간 샘플링 → 본별 로컬 포즈 행렬
    // 위치/스케일 Lerp
    // 회전 Slerp(Quaternion). 
    // 시간은 duration 으로 래핑(루프).
    class AnimClip
    {
    public:
        std::string name;
        float duration       = 0.0f; // tick 단위
        float ticksPerSecond = 1.0f;
        std::vector<AnimChannel> channels;

        // _timeSec(초)을 클립 시간으로 변환, 래핑해 본별 로컬 포즈 TRS 계산
        // 채널 없는 본/빈 트랙은 skeleton 의 localBindPose 유지
        
        void SampleTRS(float _timeSec, const Skeleton& _skeleton, LocalPoseTRS& _outPose) const;

        // 루트모션 처리용
        void SampleTRS(int _rootBoneIdx, float _timeTick, const Skeleton& _skeleton, BoneTRS& _outBone) const;
        
        float ClipDurationSec();
    };

    // 스켈레톤의 바인드 포즈를 TRS 로 분해해 채운다(클립 -1 = 정지 포즈 블렌드용).
    void SampleBindPoseTRS(const Skeleton& _skeleton, LocalPoseTRS& _outPose);

    // 두 포즈를 성분별 보간: pos/scale Lerp, rot Slerp. _t=0 → _a, _t=1 → _b. (§9)
    // _out 은 _a/_b 와 같은 본 수여야 한다(_a 를 in-place _out 으로 넘겨도 안전).
    void BlendPose(const LocalPoseTRS& _a, const LocalPoseTRS& _b, float _t, LocalPoseTRS& _out);

    // TRS 포즈 → 로컬 행렬(S·R·T, row-vector — Transform::GetMatrix 규약).
    void ComposePose(const LocalPoseTRS& _pose, std::vector<Matrix>& _outLocalPose);
}
