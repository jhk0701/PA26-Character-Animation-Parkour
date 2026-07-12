#pragma once
#include <string>

namespace MiniEngine
{
    class Skeleton;

    enum class HumanoidBone
    {
        None,
        Hips, Spine, Chest, UpperChest, Neck, Head,           // 중심(사이드 무관)
        LeftShoulder, LeftUpperArm, LeftLowerArm, LeftHand,   // 좌
        LeftUpperLeg, LeftLowerLeg, LeftFoot, LeftToes,
        RightShoulder, RightUpperArm, RightLowerArm, RightHand, // 우
        RightUpperLeg, RightLowerLeg, RightFoot, RightToes,
    };

    std::string NormalizeBoneName(std::string _name);

    // 정규화 이름(NormalizeBoneName 결과) → 휴머노이드 역할. 인식 실패 시 None.
    HumanoidBone ResolveHumanoidBone(const std::string& _normalized);

    int FindRootMotionBone(const Skeleton& _skeleton);
}
