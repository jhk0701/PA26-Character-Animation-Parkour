#pragma once
#include <string>
#include <vector>
#include <cstdint>
#include <unordered_map>     // AnalyzeRetarget/RetargetAnims 오버라이드 맵(소스본→타깃인덱스)
#include "Asset/Skeleton.h"  // RetargetAnims 타깃 스켈레톤 — 전 구성 안전(Assimp 미참조)
#include "Asset/AnimClip.h"  // RetargetAnims 결과 클립 벡터 — 전 구성 안전(Assimp 미참조)

// 원본 모델(.fbx/.gltf/.obj 등) → .mini 베이커.
namespace MiniEngine
{
    namespace Editor
    {
        
        enum class BakeUpAxis
        {
            Auto,   // assimp 축 변환을 신뢰 — 추가 회전 없음(기본값, 정상 FBX 전부)
            YUp,    // 이미 Y-up 이라고 단언 — Auto 와 동일(추가 회전 없음)
            ZUp,    // 베이크 공간의 up 이 +Z → +Y 로 세운다
            NegZUp, // 베이크 공간의 up 이 -Z → +Y 로 세운다
        };

        enum class BakeForwardAxis
        {
            Auto,  // rest 에서 실측해 +Z 로 세운다(기본값)
            PosZ,  // 이미 +Z 정면이라고 단언 — 추가 회전 없음
            NegZ,  // 정면이 -Z → yaw 180°
            PosX,  // 정면이 +X → yaw -90°
            NegX,  // 정면이 -X → yaw +90°
        };

        struct BakeOptions
        {
            BakeUpAxis      upAxis = BakeUpAxis::Auto;
            BakeForwardAxis forwardAxis = BakeForwardAxis::Auto;
        };

        // 베이크 결과 요약(UI 표시/로깅용).
        struct BakeResult
        {
            bool        success = false;
            bool        skinned = false; // SkinnedMesh 로 베이크됐는지(자동 감지 결과)
            uint32_t    vertexCount = 0;
            uint32_t    indexCount = 0;
            uint32_t    boneCount = 0;     // skinned 일 때만 유효
            uint32_t    clipCount = 0;     // skinned 일 때만 유효
            std::string message;      // 성공/실패 사유
            std::string detectedUp;
            std::string detectedForward;
        };

        // 소스 애니(FBX)의 한 채널(본)이 타깃 스켈레톤의 어느 본에 어떻게 매칭됐는지
        struct RetargetChannelMatch
        {
            std::string sourceBone;      // 소스 애니 채널 본 이름(원본 — 정규화 전, 오버라이드 키)
            int         targetBone = -1; // 자동 매칭된 타깃 본 인덱스(-1 = 미매칭)
            std::string targetBoneName;  // 타깃 본 이름(표시용; 미매칭이면 빈 문자열)
            enum class Kind { Exact, Role, Unmatched } kind = Kind::Unmatched; // 매칭 방식
        };

        // AnalyzeRetarget 결과 — 소스 본별 매칭(중복 제거) + 요약.
        struct RetargetReport
        {
            bool                              success = false;
            std::vector<RetargetChannelMatch> channels;   // 소스 본(채널) 단위, 중복 제거
            int                               matched = 0;
            int                               unmatched = 0;
            std::string                       message;     // 요약/실패 사유(UTF-8)
        };

        struct RetargetValidation
        {
            bool           success = false;
            RetargetReport report;            // 매칭 결과(Exact/Role/Unmatched) — AnalyzeRetarget 와 동일
            std::string    clipName;          // .mini 에 들어갈 이름(= 소스 파일 stem)
            uint32_t       clipCount = 0;
            float          durationSec = 0.0f;

            bool  aligned = false;
            float heightRatio = 0.0f;         // 루트 이동 스케일(동일 리그면 ≈1.0)

            float maxLimbAngleDeg = -1.0f;
            float meanLimbAngleDeg = -1.0f;
            int   limbSamples = 0;       // (사지 구간 × 프레임) 표본 수, 0 = 미측정

            std::string detectedUp;           // 소스 캐릭터 up 실측(BakeResult::detectedUp 과 동형)
            float       rootMotionNet[3] = { 0.0f, 0.0f, 0.0f }; // 루트모션 순변위(타깃 루트본 로컬)
            std::string message;
        };

        class AssimpBaker
        {
        public:
            static BakeResult Bake(const std::wstring& _srcPath,
                const std::wstring& _outMiniPath,
                const std::vector<std::wstring>& _extraAnimSources = {},
                const BakeOptions& _options = {});

            
            static BakeResult RetargetAnims(const Skeleton& _targetSkeleton,
                const std::vector<std::wstring>& _animSources,
                std::vector<AnimClip>& _inoutClips,
                const BakeOptions& _options = {});

            static RetargetReport AnalyzeRetarget(const Skeleton& _targetSkeleton,
                const std::wstring& _animSource);

            static BakeResult RetargetAnims(const Skeleton& _targetSkeleton,
                const std::wstring& _animSource,
                const std::unordered_map<std::string, int>& _overrides,
                std::vector<AnimClip>& _inoutClips,
                const BakeOptions& _options = {});

            static RetargetValidation ValidateRetarget(const Skeleton& _targetSkeleton,
                const std::wstring& _animSource);
        };
    }
}
