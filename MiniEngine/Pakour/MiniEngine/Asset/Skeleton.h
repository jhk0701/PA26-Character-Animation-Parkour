#pragma once
#include <string>
#include <vector>
#include <cassert>
#include "Core/Math.h"

namespace MiniEngine
{
    // 본 1개의 런타임 표현. 인덱스 기반 계층(부모는 항상 자기보다 앞 인덱스).
    struct Bone
    {
        int         parentIndex = -1; // -1 = 루트
        Matrix      localBindPose;    // 부모 기준 로컬 바인드 포즈
        Matrix      inverseBindPose;  // 글로벌 바인드 포즈의 역행렬
        std::string name;
    };

    // 본 계층 + 바인드 포즈. 로컬 포즈 배열 → 스키닝 최종 행렬 계산. 
    class Skeleton
    {
    public:
        std::vector<Bone> bones;

        // 로컬 포즈(본별 로컬 행렬) → 스키닝 최종 행렬.
        //   global[i] = local[i] * global[parent]   (row-vector: 자식 로컬 먼저 — SceneComponent::GetWorldMatrix 규약과 동일)
        //   outFinal[i] = inverseBindPose[i] * global[i]
        // 전제: 부모 인덱스 < 자기 인덱스 (베이크/절차 생성 시 보장).
        void ComputeBoneMatrices(const std::vector<Matrix>& _localPose, std::vector<Matrix>& _outFinal) const
        {
            const size_t count = bones.size();
            assert(_localPose.size() == count);

            // global 포즈를 outFinal 에 in-place 로 누적 후 inverseBindPose 를 곱한다.
            _outFinal.resize(count);
            for (size_t i = 0; i < count; ++i)
            {
                const int parent = bones[i].parentIndex;
                assert(parent < static_cast<int>(i));
                _outFinal[i] = (parent < 0)
                    ? _localPose[i]
                    : _localPose[i] * _outFinal[parent];
            }
            for (size_t i = 0; i < count; ++i)
                _outFinal[i] = bones[i].inverseBindPose * _outFinal[i];
        }
        
        bool IsRoot(int _idx) const { return bones[_idx].parentIndex == -1; };
    };
}
