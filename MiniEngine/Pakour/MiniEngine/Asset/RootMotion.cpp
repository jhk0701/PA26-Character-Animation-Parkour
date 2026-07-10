#include "pch.h"
#include "Asset/RootMotion.h"

namespace MiniEngine
{
	namespace
	{
		const Quaternion kIdentity(0.0f, 0.0f, 0.0f, 1.0f);
		Quaternion Inv(const Quaternion& _q)  // 단위 쿼터니언 역
		{
			Quaternion out;
			_q.Conjugate(out);
			return out;
		}
	}

	Quaternion ExtractYaw(const Quaternion& _q)
	{
		Quaternion yaw(0.0f, _q.y, 0.0f, _q.w);

		if (yaw.y * yaw.y + yaw.w * yaw.w < 1e-12f)
			return kIdentity;  // 정확히 180도 인 경우

		yaw.Normalize();
		return yaw;
	}

	void BlendRootMotion(const RootMotionDelta& _a, const RootMotionDelta& _b, float _t, RootMotionDelta& _out)
	{
		_out.translation = Vector3::Lerp(_a.translation, _b.translation, _t);
		_out.rotation = Quaternion::Lerp(_a.rotation, _b.rotation, _t);
	}

	void ExtractClipRootMotion(const AnimClip& _clip, 
		const Skeleton& _skel, 
		float _t0, float _t1, 
		RootMotionDelta& _outDelta, 
		int _rootBone)
	{
		if (_rootBone < 0 || _rootBone > static_cast<int>(_skel.bones.size()))
			return;

		const float dur = _clip.duration;
		if (dur <= 1e-6f || _t0 == _t1)
			return;

		_outDelta.Reset();

		const float tps = (_clip.ticksPerSecond > 0.0f) ? _clip.ticksPerSecond : 1.0f;
		const float t0 = _t0 * tps;
		const float t1 = _t1 * tps;

		// 시간 정규화
		const float n0 = std::floor(t0 / dur);
		const float n1 = std::floor(t1 / dur);
		const int n = static_cast<int>(n1 - n0);

		BoneTRS b0, b1;
		_clip.SampleTRS(_rootBone, t0 - n0 * dur, _skel, b0);
		_clip.SampleTRS(_rootBone, t1 - n1 * dur, _skel, b1);

		Vector3 trans = b1.pos - b0.pos;
		Quaternion rot = ExtractYaw(b1.rot) * Inv(ExtractYaw(b0.rot));

		if (n != 0) 
		{
			//  애니메이션이 루프라 한 바퀴를 더 돈 경우
			BoneTRS bStart, bEnd;
			_clip.SampleTRS(_rootBone, 0.0f, _skel, bStart);
			_clip.SampleTRS(_rootBone, dur, _skel, bEnd);

			const Vector3 cycleT = bEnd.pos - bStart.pos;
			const Quaternion cycleQ = ExtractYaw(bEnd.rot) * Inv(ExtractYaw(bStart.rot));

			for (int i = 0; i < n; ++i) { trans += cycleT; rot = cycleQ * rot; }
			for (int i = 0; i > n; --i) { trans -= cycleT; rot = Inv(cycleQ) * rot; }
		}

		rot.Normalize();
		_outDelta.translation = trans;
		_outDelta.rotation = rot;
	}

	void ApplyRootMotionMask(const RootMotionConfig& _config, RootMotionDelta& _inout)
	{
		if (!_config.extractY)
			_inout.translation.y = 0.0f;
		if (!_config.extractYaw)
			_inout.rotation = kIdentity;

	}

	void StripRootMotionFromPose(const Skeleton& _skel, const RootMotionConfig& _config, LocalPoseTRS& _inoutPose, int _rootBone)
	{
		if (_rootBone < 0 || _rootBone >= static_cast<int>(_inoutPose.size()))
			return;

		Matrix bind = _skel.bones[_rootBone].localBindPose;
		Vector3 bindScale, bindPos;
		Quaternion bindRot;
		bind.Decompose(bindScale, bindRot, bindPos);

		BoneTRS& bone = _inoutPose[_rootBone];
		bone.pos.x = bindPos.x;
		bone.pos.z = bindPos.z;

		if (_config.extractY)
			bone.pos.y = bindPos.y; // position y값 추출

		if (_config.extractYaw) 
		{
			// yaw 축 회전 제거
			bone.rot = bone.rot * Inv(ExtractYaw(bone.rot));
			bone.rot.Normalize();
		}
	}
}