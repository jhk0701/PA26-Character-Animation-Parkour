#include "pch.h"
#include "Asset/AnimClip.h"
#include <cmath>

namespace MiniEngine
{
    namespace
    {
        // 키 배열에서 _time 을 감싸는 두 키를 찾아 보간
        Vector3 SampleVec(const std::vector<VecKey>& _keys, float _time, const Vector3& _fallback)
        {
            if (_keys.empty())
                return _fallback;
            if (_time <= _keys.front().time || _keys.size() == 1)
                return _keys.front().value;
            if (_time >= _keys.back().time)
                return _keys.back().value;

            size_t next = 1;
            while (next < _keys.size() && _keys[next].time < _time)
                ++next;
            const VecKey& a = _keys[next - 1];
            const VecKey& b = _keys[next];
            const float span = b.time - a.time;
            const float t = (span > 0.0f) ? (_time - a.time) / span : 0.0f;
            return Vector3::Lerp(a.value, b.value, t);
        }

        Quaternion SampleQuat(const std::vector<QuatKey>& _keys, float _time, const Quaternion& _fallback)
        {
            if (_keys.empty())
                return _fallback;
            if (_time <= _keys.front().time || _keys.size() == 1)
                return _keys.front().value;
            if (_time >= _keys.back().time)
                return _keys.back().value;

            size_t next = 1;
            while (next < _keys.size() && _keys[next].time < _time)
                ++next;

            const QuatKey& a = _keys[next - 1];
            const QuatKey& b = _keys[next];
            const float span = b.time - a.time;
            const float t = (span > 0.0f) ? (_time - a.time) / span : 0.0f;

            return Quaternion::Slerp(a.value, b.value, t);
        }
    }

    void AnimClip::SampleTRS(float _timeSec, const Skeleton& _skeleton, LocalPoseTRS& _outPose) const
    {
        const size_t boneCount = _skeleton.bones.size();

        // 기본값 = 바인드 포즈 분해 성분 (채널 없는 본은 그대로 유지).
        SampleBindPoseTRS(_skeleton, _outPose);

        // 초 → tick 변환 후 duration 으로 래핑(루프 재생).
        float timeTick = _timeSec * ((ticksPerSecond > 0.0f) ? ticksPerSecond : 1.0f);
        if (duration > 0.0f)
        {
            timeTick = std::fmod(timeTick, duration);
            if (timeTick < 0.0f)
                timeTick += duration;
        }

        for (const AnimChannel& channel : channels)
        {
            if (channel.boneIndex < 0 || channel.boneIndex >= static_cast<int>(boneCount))
                continue;

            // 비어있는 트랙의 기본 성분은 바인드 포즈(위에서 채운 값)를 유지한다.
            BoneTRS& out = _outPose[channel.boneIndex];
            out.pos   = SampleVec(channel.pos, timeTick, out.pos);
            out.rot   = SampleQuat(channel.rot, timeTick, out.rot);
            out.scale = SampleVec(channel.scale, timeTick, out.scale);
        }
    }

    void AnimClip::Sample(float _timeSec, const Skeleton& _skeleton, std::vector<Matrix>& _outLocalPose) const
    {
        LocalPoseTRS pose;
        SampleTRS(_timeSec, _skeleton, pose);
        ComposePose(pose, _outLocalPose);
    }

    float AnimClip::ClipDurationSec()
    {
        const float tps = ticksPerSecond > 0.0f ? ticksPerSecond : 1.0f;
        return duration / tps;
    }

    void SampleBindPoseTRS(const Skeleton& _skeleton, LocalPoseTRS& _outPose)
    {
        const size_t boneCount = _skeleton.bones.size();
        _outPose.resize(boneCount);
        for (size_t i = 0; i < boneCount; ++i)
        {
            // Decompose 는 non-const 멤버 — 복사본에서 호출.
            Matrix bindPose = _skeleton.bones[i].localBindPose;
            bindPose.Decompose(_outPose[i].scale, _outPose[i].rot, _outPose[i].pos);
        }
    }

    void BlendPose(const LocalPoseTRS& _a, const LocalPoseTRS& _b, float _t, LocalPoseTRS& _out)
    {
        assert(_a.size() == _b.size());
        const size_t boneCount = _a.size();
        _out.resize(boneCount);
        for (size_t i = 0; i < boneCount; ++i)
        {
            _out[i].pos   = Vector3::Lerp(_a[i].pos, _b[i].pos, _t);
            _out[i].rot   = Quaternion::Slerp(_a[i].rot, _b[i].rot, _t); // 최단호 내장
            _out[i].scale = Vector3::Lerp(_a[i].scale, _b[i].scale, _t);
        }
    }

    void ComposePose(const LocalPoseTRS& _pose, std::vector<Matrix>& _outLocalPose)
    {
        _outLocalPose.resize(_pose.size());
        for (size_t i = 0; i < _pose.size(); ++i)
        {
            // 로컬 행렬 = S * R * T (row-vector — Transform::GetMatrix 와 동일 규약).
            _outLocalPose[i] =
                Matrix::CreateScale(_pose[i].scale)
              * Matrix::CreateFromQuaternion(_pose[i].rot)
              * Matrix::CreateTranslation(_pose[i].pos);
        }
    }
}
