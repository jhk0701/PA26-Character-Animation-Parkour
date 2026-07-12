#pragma once
#include <string>
#include <vector>
#include <cstdint>
#include <unordered_map>     // AnalyzeRetarget/RetargetAnims 오버라이드 맵(소스본→타깃인덱스)
#include "Asset/Skeleton.h"  // RetargetAnims 타깃 스켈레톤 — 전 구성 안전(Assimp 미참조)
#include "Asset/AnimClip.h"  // RetargetAnims 결과 클립 벡터 — 전 구성 안전(Assimp 미참조)

// 원본 모델(.fbx/.gltf/.obj 등) → .mini 베이커.
// **Editor 구성(WITH_EDITOR)에서만 실제로 Assimp 를 링크/사용**하며, 그 외 구성
// (Debug/Release)에서는 no-op 스텁으로 컴파일된다(assimp 심볼 미참조). → §4/§14.2 격리.
//
// 이 헤더는 구성 중립이다: assimp/MiniFormat 헤더를 끌어오지 않고 시그니처만 노출하므로
// Debug/Release 코드(예: GameCore)에서도 안전하게 include/호출할 수 있다.
// (Skeleton/AnimClip 은 전 구성 공용 런타임 타입 — Assimp 미참조라 격리 성립.)
namespace MiniEngine
{
    namespace Editor
    {
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
        };

        // ── 리타게팅 매칭 프리뷰(진단) ──────────────────────────────────────────
        // 소스 애니(FBX)의 한 채널(본)이 타깃 스켈레톤의 어느 본에 어떻게 매칭됐는지.
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

        class AssimpBaker
        {
        public:
            // _srcPath 원본 모델을 Assimp 로 임포트해 _outMiniPath 에 .mini 로 직렬화한다.
            // **자동 감지**: 소스에 본(aiBone)이 있으면 SkinnedMesh(스키닝 정점 + 스켈레톤 +
            // AnimClip 통합 컨테이너), 없으면 기존처럼 StaticMesh 로 병합 베이크.
            // _extraAnimSources: 애니메이션 FBX 목록 — 각 소스의 클립을 같은 .mini 에 병합 임베드.
            // 소스 스켈레톤이 타깃과 달라도 **베이크 타임 리타게팅**(정규화 본 이름 매핑 +
            // 회전 바인드 델타 보정 + 루트 높이 비율 스케일)으로 타깃 스켈레톤에 맞춰 변환한다.
            // (동일 리그면 항등 — 무손실. 메시/스킨 불필요, 스키닝 경로 전용.)
            // Editor 외 구성에서는 항상 실패(success=false)를 반환한다.
            static BakeResult Bake(const std::wstring& _srcPath,
                const std::wstring& _outMiniPath,
                const std::vector<std::wstring>& _extraAnimSources = {});

            // 이미 로드한 타깃 스켈레톤에 애니메이션 소스(FBX)들을 리타게팅해 _inoutClips 뒤에
            // append 한다(**파일을 쓰지 않음** — 호출부가 편집 후 WriteSkinnedMesh 로 저장).
            // 기존 `.mini` 를 다시 임포트/재베이크하지 않고 클립만 추가하기 위한 경로(§13).
            // 리타게팅 규약은 Bake 의 extra anim 병합과 동일(정규화 이름 매핑 + 휴머노이드 역할
            // fallback + 회전 바인드 델타 보정 + 루트 높이 비율 스케일). 동일 리그면 무손실.
            // 반환 BakeResult: skinned=true, clipCount = append 후 _inoutClips 총 개수,
            //   boneCount = 타깃 본 수, message = 요약(미매칭 채널 경고 포함). 실패 시 success=false.
            // Editor 외 구성에서는 항상 실패(success=false)를 반환하고 _inoutClips 무변경.
            static BakeResult RetargetAnims(const Skeleton& _targetSkeleton,
                const std::vector<std::wstring>& _animSources,
                std::vector<AnimClip>& _inoutClips);

            // 소스 애니(FBX) 하나를 타깃 스켈레톤에 리타게팅했을 때의 **채널→본 매칭만** 미리 계산한다
            // (클립 미생성). 소스 본별(중복 제거) 자동 매칭 결과(Exact/Role/Unmatched)를 돌려주므로
            // UI 가 매칭을 표로 보여주고 사용자가 오버라이드할 수 있다. 매칭 규약은 RetargetAnims 와 동일.
            // Editor 외 구성에서는 success=false 를 반환한다.
            static RetargetReport AnalyzeRetarget(const Skeleton& _targetSkeleton,
                const std::wstring& _animSource);

            // 오버라이드 맵(**소스 본 이름 → 타깃 본 인덱스**, -1 = 스킵)을 반영해 단일 소스를
            // 리타게팅해 _inoutClips 에 append 한다(파일 미기록). 오버라이드에 없는 채널은 자동 매칭
            // (정규화 이름 → 휴머노이드 역할)으로 처리한다. AnalyzeRetarget 로 검토·교정한 매핑을 적용하는 경로.
            // Editor 외 구성에서는 실패(success=false)를 반환하고 _inoutClips 무변경.
            static BakeResult RetargetAnims(const Skeleton& _targetSkeleton,
                const std::wstring& _animSource,
                const std::unordered_map<std::string, int>& _overrides,
                std::vector<AnimClip>& _inoutClips);
        };
    }
}
