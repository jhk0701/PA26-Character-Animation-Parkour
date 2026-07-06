#pragma once
#include <string>
#include <memory>
#include <vector>
#include <cstdint>
#include "Asset/StaticMesh.h"
#include "Asset/SkinnedMesh.h"
#include "Asset/MiniFormat.h"

namespace MiniEngine
{
    // .mini 바이너리 로더 / 라이터.
    // 이 모듈 자체는 외부 임포터(Assimp)를 절대 사용하지 않는다(전 구성 링크 대상, CLAUDE.md §4/§8).
    // 원본 애셋 임포트(Assimp→정점/인덱스 추출)는 Editor 전용 AssimpBaker 가 담당하며,
    // 여기서는 그렇게 만들어진 정점/인덱스를 .mini 로 직렬화하는 WriteStaticMesh 만 제공한다.
    class MiniLoader
    {
    public:
        // .mini 파일 → StaticMesh CPU 데이터 로드. 성공 시 shared_ptr 반환, 실패 시 nullptr.
        // (GPU 리소스는 호출부에서 CreateGpuResources 로 별도 생성.)
        static std::shared_ptr<StaticMesh> LoadStaticMesh(const std::wstring& _path);

        // 정점/인덱스 배열을 _path 에 StaticMesh .mini 로 직렬화. 성공 시 true.
        // (WriteCubeMini / Editor AssimpBaker 공용 — .mini 직렬화의 단일 출처.)
        static bool WriteStaticMesh(const std::wstring& _path,
                                    const std::vector<MiniStaticVertex>& _vertices,
                                    const std::vector<uint32_t>& _indices);

        // 코드로 생성한 단위 큐브(정점 24, 면별 법선)를 _path 에 .mini 로 기록. 성공 시 true.
        // 저작권 애셋 금지 — 절차적 원시 도형만. Lambert 음영 확인용.
        static bool WriteCubeMini(const std::wstring& _path);

        // .mini 파일 → SkinnedMesh(정점/인덱스 + 스켈레톤 + 클립) CPU 데이터 로드.
        // 성공 시 shared_ptr 반환, 실패 시 nullptr. (GPU 리소스는 호출부에서 별도 생성.)
        static std::shared_ptr<SkinnedMesh> LoadSkinnedMesh(const std::wstring& _path);

        // 스키닝 정점/인덱스 + 스켈레톤 + 클립을 _path 에 SkinnedMesh .mini(통합 컨테이너)로
        // 직렬화. 성공 시 true. (WriteSkinnedTest / Editor 베이커 공용 — 직렬화의 단일 출처.)
        static bool WriteSkinnedMesh(const std::wstring& _path,
                                     const std::vector<MiniSkinnedVertex>& _vertices,
                                     const std::vector<uint32_t>& _indices,
                                     const Skeleton& _skeleton,
                                     const std::vector<AnimClip>& _clips);

        // 코드로 생성한 2-본 굴곡 박스(1×4×1, Y축 링 분할 + "wave" 클립)를 _path 에 기록.
        // 성공 시 true. GPU 스키닝/AnimClip 재생 검증용(Assimp/저작권 애셋 불필요).
        static bool WriteSkinnedTest(const std::wstring& _path);
    };
}
