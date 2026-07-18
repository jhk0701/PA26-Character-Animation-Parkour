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

        // 로컬 포즈(본별 로컬 행렬) → 글로벌 포즈.
        // 전제: 부모 인덱스 < 자기 인덱스 (베이크/절차 생성 시 보장)
        void ComputeGlobalPose(const std::vector<Matrix>& _localPose, std::vector<Matrix>& _outGlobal) const
        {
            const size_t count = bones.size();
            assert(_localPose.size() == count);

            _outGlobal.resize(count);
            for (size_t i = 0; i < count; ++i)
            {
                const int parent = bones[i].parentIndex;
                assert(parent < static_cast<int>(i));
                _outGlobal[i] = (parent < 0)
                    ? _localPose[i]
                    : _localPose[i] * _outGlobal[parent];
            }
        }

        // 글로벌 포즈 -> 스키닝 최종 행렬.
        //   outFinal[i] = inverseBindPose[i] * global[i]
        void ApplyInverseBind(const std::vector<Matrix>& _globalPose, std::vector<Matrix>& _outFinal) const
        {
            const size_t count = bones.size();
            assert(_globalPose.size() == count);

            _outFinal.resize(count);
            for (size_t i = 0; i < count; ++i)
                _outFinal[i] = bones[i].inverseBindPose * _globalPose[i];
        }

        // ik용 Global Pose 재연산
        void RecomputeGlobalPoseFrom(const std::vector<Matrix>& _localPose, int _startBone,
            std::vector<Matrix>& _inoutGlobal) const
        {
            const size_t count = bones.size();
            assert(_localPose.size() == count);
            assert(_inoutGlobal.size() == count);

            for (size_t i = (_startBone < 0) ? 0 : static_cast<size_t>(_startBone); i < count; ++i)
            {
                const int parent = bones[i].parentIndex;
                assert(parent < static_cast<int>(i));
                _inoutGlobal[i] = (parent < 0)
                    ? _localPose[i]
                    : _localPose[i] * _inoutGlobal[parent];
            }
        }

        // 로컬 포즈 → 스키닝 최종 행렬 (위 둘의 합성).
        void ComputeBoneMatrices(const std::vector<Matrix>& _localPose, std::vector<Matrix>& _outFinal) const
        {
            ComputeGlobalPose(_localPose, _outFinal); // global 을 outFinal 에 in-place 누적
            ApplyInverseBind(_outFinal, _outFinal);
        }

    };
}
