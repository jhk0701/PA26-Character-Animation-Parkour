#include "pch.h"
#include "Asset/BoneNaming.h"
#include "Asset/Skeleton.h"
#include "Core/Log.h"

#include <cctype>
#include <unordered_map>

namespace MiniEngine
{
    namespace
    {
        // 부속으로 붙은 사이드를 뗀 core 이름 모음
        enum class BoneRole
        {
            None, Hips, Spine, Chest, UpperChest, Neck, Head,
            Shoulder, UpperArm, LowerArm, Hand,
            UpperLeg, LowerLeg, Foot, Toes,
        };

        HumanoidBone CombineRoleSide(BoneRole _role, int _side) // _side: 0=none 1=left 2=right
        {
            switch (_role)
            {
            case BoneRole::Hips:       return HumanoidBone::Hips;
            case BoneRole::Spine:      return HumanoidBone::Spine;
            case BoneRole::Chest:      return HumanoidBone::Chest;
            case BoneRole::UpperChest: return HumanoidBone::UpperChest;
            case BoneRole::Neck:       return HumanoidBone::Neck;
            case BoneRole::Head:       return HumanoidBone::Head;
            default: break;
            }
            if (_side == 0) return HumanoidBone::None; // 사지인데 좌우 불명 → 매칭 불가(오매칭 방지)
            const bool L = (_side == 1);
            switch (_role)
            {
            case BoneRole::Shoulder: return L ? HumanoidBone::LeftShoulder : HumanoidBone::RightShoulder;
            case BoneRole::UpperArm: return L ? HumanoidBone::LeftUpperArm : HumanoidBone::RightUpperArm;
            case BoneRole::LowerArm: return L ? HumanoidBone::LeftLowerArm : HumanoidBone::RightLowerArm;
            case BoneRole::Hand:     return L ? HumanoidBone::LeftHand : HumanoidBone::RightHand;
            case BoneRole::UpperLeg: return L ? HumanoidBone::LeftUpperLeg : HumanoidBone::RightUpperLeg;
            case BoneRole::LowerLeg: return L ? HumanoidBone::LeftLowerLeg : HumanoidBone::RightLowerLeg;
            case BoneRole::Foot:     return L ? HumanoidBone::LeftFoot : HumanoidBone::RightFoot;
            case BoneRole::Toes:     return L ? HumanoidBone::LeftToes : HumanoidBone::RightToes;
            default: return HumanoidBone::None;
            }
        }
    }

    std::string NormalizeBoneName(std::string _name)
    {
        const size_t colon = _name.find_last_of(':');
        if (colon != std::string::npos)
            _name = _name.substr(colon + 1);
        for (char& ch : _name)
            ch = static_cast<char>(std::tolower(static_cast<unsigned char>(ch)));
        return _name;
    }

    // 1. 구분자(_/./-)로 분리된 단일 문자 사이드(_l/_r, .l/.r, l_/r_) 및 단어형(left/right) 확인
    // 2. 구분자 제거해 compact core → 별칭 테이블 exact 조회
    HumanoidBone ResolveHumanoidBone(const std::string& _normalized)
    {
        std::string s = _normalized;
        int side = 0; // 0=none 1=left 2=right

        // 1 구분자로 분리된 단일 문자 사이드 접미/접두.
        if (s.size() >= 2)
        {
            const char sep = s[s.size() - 2];
            const char lr = s[s.size() - 1];
            if (sep == '_' || sep == '.' || sep == '-')
            {
                if (lr == 'l') { side = 1; s.erase(s.size() - 2); }
                else if (lr == 'r') { side = 2; s.erase(s.size() - 2); }
            }
        }
        if (side == 0 && s.size() >= 2 && s[1] == '_')
        {
            if (s[0] == 'l') { side = 1; s.erase(0, 2); }
            else if (s[0] == 'r') { side = 2; s.erase(0, 2); }
        }

        // 2 구분자 제거 → compact core.
        std::string c;
        for (char ch : s)
            if (ch != '_' && ch != '.' && ch != '-' && ch != ' ') c += ch;

        // 3 단어형 사이드(left/right) 접두/접미.
        if (side == 0)
        {
            if (c.size() >= 4 && c.compare(0, 4, "left") == 0) { side = 1; c.erase(0, 4); }
            else if (c.size() >= 5 && c.compare(0, 5, "right") == 0) { side = 2; c.erase(0, 5); }
            else if (c.size() >= 4 && c.compare(c.size() - 4, 4, "left") == 0) { side = 1; c.erase(c.size() - 4); }
            else if (c.size() >= 5 && c.compare(c.size() - 5, 5, "right") == 0) { side = 2; c.erase(c.size() - 5); }
        }

        // 4 core → 역할. 스파인 체인은 리그마다 인덱싱이 달라 근사 매핑
        // UE5 Manny/Quinn: spine_01~05·neck_01/02·ball_l/r 커버(neck01/02, spine4/04·5/05, ball 별칭)
        static const std::unordered_map<std::string, BoneRole> table = {
            { "hips", BoneRole::Hips }, { "pelvis", BoneRole::Hips }, { "bip01pelvis", BoneRole::Hips },
            { "spine", BoneRole::Spine },
            { "spine1", BoneRole::Chest }, { "spine01", BoneRole::Chest }, { "chest", BoneRole::Chest },
            { "spine2", BoneRole::UpperChest }, { "spine02", BoneRole::UpperChest },
            { "spine3", BoneRole::UpperChest }, { "spine03", BoneRole::UpperChest }, { "upperchest", BoneRole::UpperChest },
            { "spine4", BoneRole::UpperChest }, { "spine04", BoneRole::UpperChest }, // UE5 상단 척추
            { "spine5", BoneRole::UpperChest }, { "spine05", BoneRole::UpperChest }, // UE5 상단 척추
            { "neck", BoneRole::Neck }, { "neck1", BoneRole::Neck }, { "neck01", BoneRole::Neck },
            { "neck2", BoneRole::Neck }, { "neck02", BoneRole::Neck }, // UE neck_01/02
            { "head", BoneRole::Head },
            { "shoulder", BoneRole::Shoulder }, { "clavicle", BoneRole::Shoulder },
            { "upperarm", BoneRole::UpperArm }, { "arm", BoneRole::UpperArm },
            { "lowerarm", BoneRole::LowerArm }, { "forearm", BoneRole::LowerArm },
            { "hand", BoneRole::Hand },
            { "upperleg", BoneRole::UpperLeg }, { "upleg", BoneRole::UpperLeg }, { "thigh", BoneRole::UpperLeg },
            { "lowerleg", BoneRole::LowerLeg }, { "leg", BoneRole::LowerLeg }, { "calf", BoneRole::LowerLeg }, { "shin", BoneRole::LowerLeg },
            { "foot", BoneRole::Foot },
            { "toes", BoneRole::Toes }, { "toe", BoneRole::Toes }, { "toebase", BoneRole::Toes }, { "ball", BoneRole::Toes }, // ball=UE 발가락
        };
        const auto it = table.find(c);
        if (it == table.end()) return HumanoidBone::None;
        return CombineRoleSide(it->second, side);
    }

    int FindRootMotionBone(const Skeleton& _skeleton)
    {
        int hipsIndex = -1;
        for (size_t i = 0; i < _skeleton.bones.size(); ++i)
        {
            const std::string norm = NormalizeBoneName(_skeleton.bones[i].name);
            if (norm == "root")
                return static_cast<int>(i); // 전용 루트 본 — 최우선.
            if (hipsIndex < 0 && ResolveHumanoidBone(norm) == HumanoidBone::Hips)
                hipsIndex = static_cast<int>(i);
        }

        if (hipsIndex < 0)
            MG_LOG_WARN("BoneNaming: root motion bone not found (no \"root\" bone, no Hips role)");
        return hipsIndex;
    }

    void BuildHumanoidBoneMap(const Skeleton& _skeleton, HumanoidBoneMap& _out)
    {
        for (size_t r = 0; r < static_cast<size_t>(HumanoidBone::Count); ++r)
            _out.index[r] = -1;

        for (size_t i = 0; i < _skeleton.bones.size(); ++i)
        {
            const HumanoidBone role = ResolveHumanoidBone(NormalizeBoneName(_skeleton.bones[i].name));
            if (role == HumanoidBone::None) continue;

            int& slot = _out.index[static_cast<size_t>(role)];
            if (slot < 0) slot = static_cast<int>(i); // 첫 등장 유지 — 다대일 매칭의 대표.
        }
    }

}
