#pragma once
#include <string>
#include <vector>
#include <cstdint>
#include <unordered_map>     
#include "Asset/Skeleton.h"  
#include "Asset/AnimClip.h"  

// 원본 모델(.fbx/.gltf/.obj 등) → .mini 베이커.
// Editor 구성(WITH_EDITOR)에서만 실제로 Assimp 를 링크/사용
namespace MiniEngine
{
    namespace Editor
    {
        // 임포트 옵션
        enum class BakeUpAxis
        {
            Auto,   // assimp 축 변환을 신뢰 — 추가 회전 없음(기본값, 정상 FBX 전부)
            YUp,    // 이미 Y-up 이라고 단언 — Auto 와 동일(추가 회전 없음)
            ZUp,    // 베이크 공간의 up 이 +Z → +Y 로 세운다
            NegZUp, // 베이크 공간의 up 이 -Z → +Y 로 세운다
        };

        struct BakeOptions
        {
            BakeUpAxis upAxis = BakeUpAxis::Auto;
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
            std::string message;      // 성공/실패 사유(UTF-8)
            std::string detectedUp;
        };

        // 리타게팅 매칭
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

            // 리그 정렬(캐릭터 공간 기저)이 유도됐는지. false = Hips/Head/UpperLeg 역할 본을 못 찾아
            // 원시 모델스페이스 전이로 폴백 → 크로스 리그면 사지가 깨진다.
            bool  aligned = false;
            float heightRatio = 0.0f;         // 루트 이동 스케일(동일 리그면 ≈1.0)

            // **핵심 지표** — 소스 사지 방향 vs 리타게팅 결과(각자 캐릭터 공간) 각도, 도 단위.
            // 리타게팅 공식이 성립시키려는 항등의 잔차라 **0 이 정답**이다.
            // 실측 이력(docs/UEFN_Bone.md §5.6): 공식이 깨졌을 때 149°/170°, 고친 뒤 0.02°.
            // -1 = 측정 불가(aligned=false 또는 사지 역할 본 부재).
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
            // _srcPath 원본 모델을 Assimp 로 임포트해 _outMiniPath 에 .mini 로 직렬화
            static BakeResult Bake(const std::wstring& _srcPath,
                const std::wstring& _outMiniPath,
                const std::vector<std::wstring>& _extraAnimSources = {},
                const BakeOptions& _options = {});

            // 이미 로드한 타깃 스켈레톤에 애니메이션 소스(FBX)들을 리타게팅
            static BakeResult RetargetAnims(const Skeleton& _targetSkeleton,
                const std::vector<std::wstring>& _animSources,
                std::vector<AnimClip>& _inoutClips);

            static RetargetReport AnalyzeRetarget(const Skeleton& _targetSkeleton,
                const std::wstring& _animSource);

            static BakeResult RetargetAnims(const Skeleton& _targetSkeleton,
                const std::wstring& _animSource,
                const std::unordered_map<std::string, int>& _overrides,
                std::vector<AnimClip>& _inoutClips);

            static RetargetValidation ValidateRetarget(const Skeleton& _targetSkeleton,
                const std::wstring& _animSource);
        };
    }
}
