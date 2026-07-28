#pragma once
#include <string>

namespace MiniEngine
{
    class Skeleton;

    enum class HumanoidBone : uint8_t
    {
        None,
        Hips, Spine, Chest, UpperChest, Neck, Head,           // 중심(사이드 무관)
        LeftShoulder, LeftUpperArm, LeftLowerArm, LeftHand,   // 좌
        LeftUpperLeg, LeftLowerLeg, LeftFoot, LeftToes,
        RightShoulder, RightUpperArm, RightLowerArm, RightHand, // 우
        RightUpperLeg, RightLowerLeg, RightFoot, RightToes,
        
        Count
    };

    std::string NormalizeBoneName(std::string _name);

    // 정규화 이름(NormalizeBoneName 결과) → 휴머노이드 역할. 인식 실패 시 None.
    HumanoidBone ResolveHumanoidBone(const std::string& _normalized);

    int FindRootMotionBone(const Skeleton& _skeleton);

    struct HumanoidBoneMap
    {
        int index[static_cast<size_t>(HumanoidBone::Count)] = {}; // BuildHumanoidBoneMap 이 -1 로 초기화
        int  Get(HumanoidBone _role) const
        {
            if (_role == HumanoidBone::None || _role >= HumanoidBone::Count) return -1;
            return index[static_cast<size_t>(_role)];
        }
        bool Has(HumanoidBone _role) const { return Get(_role) >= 0; }
    };

    // 리타겟시, 본 구조를 읽고 매핑
    void BuildHumanoidBoneMap(const Skeleton& _skeleton, HumanoidBoneMap& _out);
}
