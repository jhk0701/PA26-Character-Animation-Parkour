#include "pch.h"
#include "Asset/IK.h"
#include <cmath>

namespace MiniEngine 
{
	namespace 
	{
		constexpr float EPS = 1e-6f;

		inline Quaternion Identity() { return Quaternion(0.0f, 0.0f, 0.0f, 1.0f); }

		float ClampReach(float _dist, float _lenU, float _lenL) 
		{
			const float LO = std::fabs(_lenU - _lenL) + EPS;
			const float HI = _lenU + _lenL - EPS;

			if (HI <= LO)
				return LO;

			return max(LO, min(HI, _dist));
		}
	}

	Quaternion FromToRotation(const Vector3& _from, const Vector3& _to)
	{
		// 둘 중 하나라로 길이가 0인 경우 예외
		if (_from.LengthSquared() < EPS * EPS || _to.LengthSquared() < EPS * EPS)
			return Identity();

		Vector3 f = _from;
		Vector3 t = _to;
		f.Normalize(); t.Normalize();

		float d = f.Dot(t);
		d = max(-1.0f, min(1.0f, d));

		if (d > 1.0f - 1e-5f) // 이미 일치함
			return Identity();

		if (d < -1.0f + 1e-5f)
		{
			// 정확히 반대인 상황
			// from에 수직인 아무 축으로 180도 회전
			Vector3 axis = Vector3(1.0f, 0.0f, 0.0f).Cross(f);
			
			if (axis.LengthSquared() < EPS)
				axis = Vector3(0.0f, 1.0f, 0.0f).Cross(f);

			axis.Normalize();

			return Quaternion::CreateFromAxisAngle(axis, PI);
		}

		Vector3 axis = f.Cross(t);
		axis.Normalize();
		return Quaternion::CreateFromAxisAngle(axis, std::acos(d));
	}


	bool FallbackPole(const TwoBoneIKBone& _inBone, Vector3& _outPole)
	{
		Vector3 axis = _inBone.endPos - _inBone.upperPos;
		
		const float AXIS_LEN = axis.Length();
		if (AXIS_LEN < EPS)
			return false; // upper 와 end 가 겹침 -> 평면 미정

		axis /= AXIS_LEN;

		const Vector3 V = _inBone.lowerPos - _inBone.upperPos;
		const Vector3 PERP = V - axis * V.Dot(axis);

		// 상대 임계값: 본 길이에 비례해 판정해야 리그 단위(cm/m)에 무관
		if (PERP.Length() < 1e-4f * max(AXIS_LEN, V.Length()))
			return false;

		_outPole = _inBone.lowerPos;
		return true;
	}

	bool SolveTwoBone(const TwoBoneIKBone& _inBone, const TwoBoneIKTarget& _inTarget, TwoBoneIKResult& _outResult)
	{
		_outResult.upperDelta = Identity();
		_outResult.lowerDelta = Identity();
		
		const float LEN_U = (_inBone.lowerPos - _inBone.upperPos).Length();
		const float LEN_L = (_inBone.endPos - _inBone.lowerPos).Length();

		if (LEN_U < EPS || LEN_L < EPS)
			return false; // 본 길이 0인 경우

		Vector3 dir = _inTarget.targetPos - _inBone.upperPos;
		const float DIST_RAW = dir.Length();

		if (DIST_RAW < EPS)
			return false;

		dir /= DIST_RAW; 
		const float DIST = ClampReach(DIST_RAW, LEN_U, LEN_L);

		Vector3 bendNormal = dir.Cross(_inTarget.poleTargetPos - _inBone.upperPos);
		if (bendNormal.Length() < EPS)
			return false; // 폴의 방향과 dir이 평행한 경우

		// 굽힘 평면
		bendNormal.Normalize();
		const Vector3 BEND_DIR = bendNormal.Cross(dir); // dir에 수직, 폴 쪽 반 평면

		// upper 관절 개방각
		float cosA = (LEN_U * LEN_U + DIST * DIST - LEN_L * LEN_L) / (2.0f * LEN_U * DIST);
		cosA = max(-1.0f, min(1.0f, cosA));
		const float ANGLE_A = std::acos(cosA);

		const Vector3 NEW_LOWER = _inBone.upperPos + (dir * cos(ANGLE_A) + BEND_DIR * sin(ANGLE_A)) * LEN_U;
		const Vector3 NEW_END = _inBone.upperPos + dir * DIST;

		// 위치 -> 델타 회전
		const Quaternion Q_U = FromToRotation(_inBone.lowerPos - _inBone.upperPos, NEW_LOWER - _inBone.upperPos);
		
		// lower 델타는 upper 델타가 이미 적용된 중간 상태를 기준으로 해야함
		const Vector3 END_AFTER_U = _inBone.upperPos + Vector3::Transform(_inBone.endPos - _inBone.upperPos, Q_U);
		const Quaternion Q_L = FromToRotation(END_AFTER_U - NEW_LOWER, NEW_END - NEW_LOWER);

		const float ALPHA = std::clamp(_inTarget.alpha, 0.0f, 1.0f);
		_outResult.upperDelta = Quaternion::Slerp(Identity(), Q_U, ALPHA);
		_outResult.lowerDelta = Quaternion::Slerp(Identity(), Q_L, ALPHA);

		return true;
	}
}