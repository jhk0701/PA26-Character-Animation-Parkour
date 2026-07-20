#pragma once
#include <cstdint>

// .mini 바이너리 애셋 포맷
//
// 파일 레이아웃 (리틀엔디안, x64 고정):
//   [ MiniHeader ]                     // 16바이트 (magic/version/assetType/reserved)
//   [ 타입별 본문 ]
//
// StaticMesh
//   [ MiniStaticMeshHeader ]           // 8바이트 (vertexCount/indexCount)
//   [ MiniStaticVertex   * vertexCount ] // 32바이트 * N (pos3/normal3/uv2)
//   [ uint32             * indexCount  ] // 4바이트 * M
//
// SkinnedMesh 스키닝 메시 + 스켈레톤 + AnimClip
//   [ MiniSkinnedMeshHeader ]            // 16바이트 (vertexCount/indexCount/boneCount/clipCount)
//   [ MiniSkinnedVertex  * vertexCount ] // 64바이트 * N (pos3/normal3/uv2 + boneIndices4/boneWeights4)
//   [ uint32             * indexCount  ] // 4바이트 * M
//   [ MiniBone           * boneCount   ] // 196바이트 * B (부모 인덱스는 항상 자기보다 앞)
//   [ 클립 섹션          * clipCount   ] // 클립마다:
//       [ MiniAnimClipHeader ]                       // 76바이트 (name/duration/tps/channelCount)
//       [ 채널 * channelCount ] 채널마다:
//           [ MiniAnimChannelHeader ]                // 16바이트 (boneIndex/키 개수 3종)
//           [ MiniVecKey  * posKeyCount   ]          // 16바이트 * K
//           [ MiniQuatKey * rotKeyCount   ]          // 20바이트 * K
//           [ MiniVecKey  * scaleKeyCount ]          // 16바이트 * K
//
// 직렬화 안정성: 모든 멤버를 고정폭 정수/float 로 두고 #pragma pack(4)로
// 패딩을 제거해 컴파일러/구성 간 레이아웃을 일치시킨다.

namespace MiniEngine
{
    // 'MINI' (리틀엔디안 파일에서 바이트열 'M','I','N','I' 로 기록)
    constexpr uint32_t MINI_MAGIC   = 0x494E494D; // 'I''N''I''M' → 파일상 "MINI"
    constexpr uint32_t MINI_VERSION = 1;

    // 애셋 타입
    // SkinnedMesh 는 통합 컨테이너(메시+스켈레톤+클립).
    // Skeleton/AnimClip 단독 파일 슬롯은 추후 분할용으로 보존.
    enum class MiniAssetType : uint32_t
    {
        Unknown,
        StaticMesh,
        SkinnedMesh,
        Skeleton,
        AnimClip,
    };

    // 본/클립 이름의 고정 길이(널 종단 포함)
    constexpr uint32_t MINI_NAME_LENGTH = 64;
    constexpr uint32_t MINI_BAKE_AXIS_NORMALIZED = 1u << 0;

#pragma pack(push, 4)

    // 모든 .mini 파일 공통 헤더 (16바이트)
    struct MiniHeader
    {
        uint32_t magic;     // == MINI_MAGIC
        uint32_t version;   // == MINI_VERSION
        uint32_t assetType; // MiniAssetType 값
        uint32_t bakeFlags; // MINI_BAKE_* 비트 OR (구 파일 = 0). 구: reserved.
    };

    // StaticMesh 본문 헤더 (8바이트)
    struct MiniStaticMeshHeader
    {
        uint32_t vertexCount;
        uint32_t indexCount;
    };

    // StaticMesh 정점 (32바이트)
    // pos/normal/uv.
    struct MiniStaticVertex
    {
        float position[3];
        float normal[3];
        float uv[2];
    };

    // SkinnedMesh 본문 헤더 (16바이트)
    struct MiniSkinnedMeshHeader
    {
        uint32_t vertexCount;
        uint32_t indexCount;
        uint32_t boneCount;
        uint32_t clipCount;
    };

    // 스킨드 버텍스 (64바이트)
    // 앞 32바이트는 MiniStaticVertex 와 동일 레이아웃.
    struct MiniSkinnedVertex
    {
        float    position[3];
        float    normal[3];
        float    uv[2];
        uint32_t boneIndices[4]; // 본 인덱스 4개 (미사용 슬롯은 0)
        float    boneWeights[4]; // 가중치 4개 (합 = 1.0, 미사용 슬롯은 0)
    };

    // 본 1개 (196바이트)
    // 행렬은 SimpleMath row-major 저장 순서 그대로 16 float
    struct MiniBone
    {
        int32_t parentIndex;          // -1 = 루트. 항상 자기 인덱스보다 앞(위상 정렬).
        float   localBindPose[16];    // 부모 기준 로컬 바인드 포즈
        float   inverseBindPose[16];  // 글로벌 바인드 포즈의 역행렬
        char    name[MINI_NAME_LENGTH];
    };

    // AnimClip 헤더 (76바이트). duration/키 time 단위는 tick, 초 = tick / ticksPerSecond.
    struct MiniAnimClipHeader
    {
        char     name[MINI_NAME_LENGTH];
        float    duration;
        float    ticksPerSecond;
        uint32_t channelCount;
    };

    // 본 1개에 대한 키 트랙 헤더 (16바이트).
    struct MiniAnimChannelHeader
    {
        uint32_t boneIndex;
        uint32_t posKeyCount;
        uint32_t rotKeyCount;
        uint32_t scaleKeyCount;
    };

    // 벡터 키 (16바이트) — position/scale 공용.
    struct MiniVecKey
    {
        float time;
        float v[3];
    };

    // 쿼터니언 키 (20바이트) — rotation. v = (x,y,z,w).
    struct MiniQuatKey
    {
        float time;
        float v[4];
    };

#pragma pack(pop)

    static_assert(sizeof(MiniHeader) == 16, "MiniHeader must be 16 bytes");
    static_assert(sizeof(MiniStaticMeshHeader) == 8, "MiniStaticMeshHeader must be 8 bytes");
    static_assert(sizeof(MiniStaticVertex) == 32, "MiniStaticVertex must be 32 bytes");
    static_assert(sizeof(MiniSkinnedMeshHeader) == 16, "MiniSkinnedMeshHeader must be 16 bytes");
    static_assert(sizeof(MiniSkinnedVertex) == 64, "MiniSkinnedVertex must be 64 bytes");
    static_assert(sizeof(MiniBone) == 196, "MiniBone must be 196 bytes");
    static_assert(sizeof(MiniAnimClipHeader) == 76, "MiniAnimClipHeader must be 76 bytes");
    static_assert(sizeof(MiniAnimChannelHeader) == 16, "MiniAnimChannelHeader must be 16 bytes");
    static_assert(sizeof(MiniVecKey) == 16, "MiniVecKey must be 16 bytes");
    static_assert(sizeof(MiniQuatKey) == 20, "MiniQuatKey must be 20 bytes");
}
