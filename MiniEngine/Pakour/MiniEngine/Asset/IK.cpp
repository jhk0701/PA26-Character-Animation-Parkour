#include "pch.h"
#include "Asset/IK.h"
#include <cmath>

namespace MiniEngine 
{
	namespace 
	{
		constexpr float EPS = 1e-6f;

		// IK 떨림 보정을 위한 임계값
		constexpr float MIN_BEND_SIN = 0.05f;   // 약 2.9도
		constexpr float REACH_SOFT_START = 0.97f;   // 이 비율부터 압축 시작
		constexpr float REACH_SOFT_MAX = 0.995f;    // 점근 상한

		// 저작된 한계(굽힘각/콘)에 쓰는 소프트 구간 폭 — 밴드 대비 비율
		constexpr float LIMIT_SOFT_FRAC = 0.05f;

		inline Quaternion Identity() { return Quaternion(0.0f, 0.0f, 0.0f, 1.0f); }

		// 오버슛 _dv(>=0) 를 [0,_range) 로 지수 포화시킨다. _dv=0 -> 0, _dv->무한 -> _range 에 점근.
		// 하드 클램프를 특이점 바로 위에 붙이면 경계를 넘나들 때 팝하므로, 경계에 "도달하지 않는"
		// 곡선으로 바꾸는 것이 목적이다.
		//
		// _span 은 포화 곡선의 정규화 분모다. 접합점 기울기가 _range/_span 이므로
		// _span == _range 면 C1 연속이 된다.
		// ⚠ 기존 사거리 상한은 _span = FULL-START, _range = HI-START 라 기울기가 0.833 인
		//   (C0) 곡선이다. 여기를 _range 로 "고치면" 기존 동작이 조용히 바뀌므로 그대로 둔다.
		inline float SoftSaturate(float _dv, float _range, float _span)
		{
			if (_dv <= 0.0f || _range <= 0.0f || _span <= EPS)
				return max(0.0f, min(_dv, _range));

			return _range * (1.0f - std::exp(-_dv / _span));
		}

		// lower 관절 내각 -> upper~end 거리 (코사인 법칙).
		// B=0 -> |u-l| (완전히 접힘), B=180도 -> u+l (완전히 펴짐). 그 사이는 단조증가.
		inline float BendToDist(float _bendRad, float _lenU, float _lenL)
		{
			const float V = _lenU * _lenU + _lenL * _lenL
				- 2.0f * _lenU * _lenL * std::cos(_bendRad);

			return std::sqrt(max(0.0f, V));
		}

		float ClampReach(float _dist, float _lenU, float _lenL, const TwoBoneIKLimits& _limits)
		{
			const float FULL = _lenU + _lenL;
			float lo = std::fabs(_lenU - _lenL) + EPS;
			float hi = FULL * REACH_SOFT_MAX;

			// 굽힘각 한계를 사거리 한계로 접는다 (단조 대응이라 그대로 대입하면 된다)
			const bool B_MIN_BEND = _limits.minBendDeg > 0.0f;
			const bool B_MAX_BEND = _limits.maxBendDeg < 180.0f;

			if (B_MIN_BEND)
				lo = max(lo, BendToDist(ToRadians(_limits.minBendDeg), _lenU, _lenL));

			if (B_MAX_BEND)
				hi = min(hi, BendToDist(ToRadians(_limits.maxBendDeg), _lenU, _lenL));

			if (hi <= lo)
				return lo;

			// 소프트 상한
			// 기본 상한(완전 신전)은 dA/dDIST -> 무한 인 특이점이라 반드시 포화가 필요하다.
			// 저작 상한은 특이점이 아니지만, 하드 클램프면 경계에서 팔꿈치가 멈칫하므로 같이 포화시킨다.
			// 앵커가 다른 이유: 저작 상한이 FULL*0.97 보다 낮으면 기존 START 가 hi 를 넘어버린다.
			{
				const float START = B_MAX_BEND ? (hi - (hi - lo) * LIMIT_SOFT_FRAC)
					: (FULL * REACH_SOFT_START);
				const float SPAN = B_MAX_BEND ? (hi - START)      // C1
					: (FULL - START);   // 기존 곡선 유지(무회귀)

				if (_dist > START && START > lo)
					_dist = START + SoftSaturate(_dist - START, hi - START, SPAN);
			}

			// 소프트 하한 — 저작된 접힘 한계가 있을 때만.
			// 기본 하한 |u-l| 은 관절 한계가 아니라 기하 축퇴 가드(분모 2*u*d 발산 방지)라
			// 성격이 다르고, 여기에 포화를 넣으면 기존 동작이 바뀐다.
			if (B_MIN_BEND)
			{
				const float START = lo + (hi - lo) * LIMIT_SOFT_FRAC;
				const float SPAN = START - lo; // C1

				if (_dist < START && START < hi)
					_dist = START - SoftSaturate(START - _dist, SPAN, SPAN);
			}

			return max(lo, min(hi, _dist));
		}

		// 스윙 콘 클램프 — upper 방향을 coneAxis 둘레 원뿔 안으로 밀어 넣는다.
		// 굽힘각과 달리 사거리로 접히지 않는 "방향" 제약이라 여기서 직접 자른다.
		// 한계가 없거나 이미 콘 안이면 아무것도 하지 않는다.
		void ApplySwingCone(const Vector3& _upperPos, const Vector3& _targetPos,
			float _lenU, float _lenL, const TwoBoneIKLimits& _limits,
			Vector3& _inoutLower, Vector3& _inoutEnd)
		{
			if (_limits.swingConeDeg >= 180.0f)
				return;

			Vector3 coneAxis = _limits.coneAxis;
			if (coneAxis.LengthSquared() < EPS * EPS)
				return; // 축 미지정 — 호출자가 rest 유도에 실패한 경우

			coneAxis.Normalize();

			Vector3 uDir = _inoutLower - _upperPos;
			if (uDir.LengthSquared() < EPS * EPS)
				return;

			uDir.Normalize();

			const float CONE_RAD = ToRadians(max(0.0f, _limits.swingConeDeg));
			const float ANG = std::acos(max(-1.0f, min(1.0f, uDir.Dot(coneAxis))));

			// 소프트 상한 — 경계에 하드로 붙이면 타깃이 경계를 넘나들 때 팔꿈치가 멈칫한다
			const float START = CONE_RAD * (1.0f - LIMIT_SOFT_FRAC);
			const float SPAN = CONE_RAD - START; // C1

			if (ANG <= START)
				return; // 이미 콘 안

			const float ANG_C = START + SoftSaturate(ANG - START, SPAN, SPAN);

			Vector3 axis = coneAxis.Cross(uDir);
			if (axis.LengthSquared() < EPS * EPS)
				return; // coneAxis 와 평행 — 0도는 위에서 걸렀고 180도는 축이 미정이다

			axis.Normalize();

			// coneAxis 를 uDir 쪽으로 ANG_C 만큼 돌린다.
			// 축=cross(from,to) + 양의 각 이 from 을 to 로 보낸다는 것은 이 파일의
			// FromToRotation 이 쓰는 것과 같은 구성이다(솔버 전체가 그 위에 서 있다).
			const Vector3 U_DIR_C = Vector3::Transform(coneAxis, Quaternion::CreateFromAxisAngle(axis, ANG_C));

			_inoutLower = _upperPos + U_DIR_C * _lenU;

			// 상완이 잘렸으니 하완은 타깃을 향해 최선을 다한다 (도달은 포기)
			Vector3 toTarget = _targetPos - _inoutLower;
			if (toTarget.LengthSquared() < EPS * EPS)
			{
				_inoutEnd = _inoutLower + U_DIR_C * _lenL;
				return;
			}

			toTarget.Normalize();
			_inoutEnd = _inoutLower + toTarget * _lenL;
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

		// |PERP| = |V| * sin(각) 임계값 확인
		if (PERP.Length() < MIN_BEND_SIN * V.Length())
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
		const float DIST = ClampReach(DIST_RAW, LEN_U, LEN_L, _inTarget.limits);

		// dir 이 단위벡터이므로 |dir x v| = |v| * sin(각)
		const Vector3 V_POLE = _inTarget.poleTargetPos - _inBone.upperPos;
		const float POLE_LEN = V_POLE.Length();

		Vector3 bendNormal = dir.Cross(V_POLE);
		if (POLE_LEN < EPS || bendNormal.Length() < MIN_BEND_SIN * POLE_LEN)
			return false; // 폴의 방향과 dir이 (거의) 평행한 경우

		// 굽힘 평면
		bendNormal.Normalize();
		const Vector3 BEND_DIR = bendNormal.Cross(dir); // dir에 수직, 폴 쪽 반 평면

		// upper 관절 개방각
		float cosA = (LEN_U * LEN_U + DIST * DIST - LEN_L * LEN_L) / (2.0f * LEN_U * DIST);
		cosA = max(-1.0f, min(1.0f, cosA));
		const float ANGLE_A = std::acos(cosA);

		Vector3 newLower = _inBone.upperPos + (dir * cos(ANGLE_A) + BEND_DIR * sin(ANGLE_A)) * LEN_U;
		Vector3 newEnd = _inBone.upperPos + dir * DIST;

		// 스윙 콘 — upper 가 rest 방향(coneAxis)에서 벌어질 수 있는 각을 제한한다.
		// 굽힘각 한계와 달리 사거리로 접히지 않으므로(방향 제약이다) 여기서 직접 자른다.
		// 잘리면 end 는 타깃에 미달한다 — 제약이 있는 IK 에서는 정상이다.
		ApplySwingCone(_inBone.upperPos, _inTarget.targetPos, LEN_U, LEN_L, _inTarget.limits,
			newLower, newEnd);

		const Vector3 NEW_LOWER = newLower;
		const Vector3 NEW_END = newEnd;

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