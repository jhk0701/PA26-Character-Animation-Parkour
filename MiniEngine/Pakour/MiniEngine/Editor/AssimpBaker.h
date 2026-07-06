#pragma once
#include <string>
#include <vector>
#include <cstdint>

// 원본 모델(.fbx/.gltf/.obj 등) → .mini 베이커.
// **Editor 구성(WITH_EDITOR)에서만 실제로 Assimp 를 링크/사용**하며, 그 외 구성
// (Debug/Release)에서는 no-op 스텁으로 컴파일된다(assimp 심볼 미참조). → §4/§14.2 격리.
//
// 이 헤더는 구성 중립이다: assimp/MiniFormat 헤더를 끌어오지 않고 시그니처만 노출하므로
// Debug/Release 코드(예: GameCore)에서도 안전하게 include/호출할 수 있다.
namespace MiniEngine
{
    namespace Editor
    {
        // 베이크 결과 요약(UI 표시/로깅용).
        struct BakeResult
        {
            bool        success     = false;
            bool        skinned     = false; // SkinnedMesh 로 베이크됐는지(자동 감지 결과)
            uint32_t    vertexCount = 0;
            uint32_t    indexCount  = 0;
            uint32_t    boneCount   = 0;     // skinned 일 때만 유효
            uint32_t    clipCount   = 0;     // skinned 일 때만 유효
            std::string message;      // 성공/실패 사유(UTF-8)
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
        };
    }
}
