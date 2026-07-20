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
    // .mini 읽기, 쓰기용
    class MiniLoader
    {
    public:
        // .mini 파일 → StaticMesh CPU 데이터 로드
        // GPU 리소스는 호출부에서 CreateGpuResources로 생성
        static std::shared_ptr<StaticMesh> LoadStaticMesh(const std::wstring& _path);

        // 정점/인덱스 배열을 _path 에 StaticMesh .mini 로 직렬화
        static bool WriteStaticMesh(const std::wstring& _path,
                                    const std::vector<MiniStaticVertex>& _vertices,
                                    const std::vector<uint32_t>& _indices);

        // .mini 파일 → SkinnedMesh(정점/인덱스 + 스켈레톤 + 클립) CPU 데이터 로드
        static std::shared_ptr<SkinnedMesh> LoadSkinnedMesh(const std::wstring& _path);

        // 스키닝 정점/인덱스 + 스켈레톤 + 클립을 _path 에 SkinnedMesh .mini 직렬화
        static bool WriteSkinnedMesh(const std::wstring& _path,
                                     const std::vector<MiniSkinnedVertex>& _vertices,
                                     const std::vector<uint32_t>& _indices,
                                     const Skeleton& _skeleton,
                                     const std::vector<AnimClip>& _clips,
                                     uint32_t _bakeFlags = 0);

        // .mini 의 공통 헤더만 읽는 용도
        // bake flags확인
        static bool PeekHeader(const std::wstring& _path, MiniHeader& _outHeader);
    };
}
