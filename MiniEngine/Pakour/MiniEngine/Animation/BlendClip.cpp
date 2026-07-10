#include "pch.h"
#include "Animation/BlendClip.h"

namespace MiniEngine
{
	BlendClip::BlendClip()
	{
		m_placements.reserve(9);
		m_playTime = 0.0f;
	}
	BlendClip::BlendClip(int _reserveCnt)
	{
		m_placements.reserve(_reserveCnt);
		m_playTime = 0.0f;
	}

	void BlendClip::Sample(float _dt, const Skeleton& _skeleton, LocalPoseTRS& _outPose)
	{
		if (m_placements.empty())
			return;

		int matchedIdx = 0;
		bool bIsUnique = ComputeWeight(matchedIdx);
		
		m_playTime += _dt;

		if (bIsUnique)
		{
			// 선택한 모션 즉시 반영
			m_placements[matchedIdx].m_pClip->SampleTRS(m_playTime, _skeleton, _outPose);
			return;
		}

		bool bIsFirst = true;
		float accW = 0.0f;
		for (const Placement& p : m_placements)
		{
			if (p.m_weight <= 0.0f)
				continue;

			// 이 클립의 현재 시간대의 애니메이션 본 트랜스폼 반영
			p.m_pClip->SampleTRS(m_playTime, _skeleton, m_poseScratch); // 임시 포즈에 현재 클립 원본 자세 보관

			if (bIsFirst) 
			{
				m_blendedPose = m_poseScratch;
				accW = p.m_weight;
				bIsFirst = false;
				continue;
			}
			else
			{
				// TODO : 다중 모션 가중치에 따른 블렌드 공식 찾아볼 것
				// 현재 가중치에 따라 희석되는 식으로 구현
				p.m_pClip->SampleTRS(m_playTime, _skeleton, m_poseScratch);
				BlendPose(m_blendedPose, m_poseScratch, p.m_weight / (accW + p.m_weight), m_blendedPose);

				accW += p.m_weight;
			}
		}

		_outPose = m_blendedPose; // 정확히 하나에만 해당하는 값이므로 바로 변영
	}

	void BlendClip::SetAxisValue(float _x, float _y)
	{
		m_AxisX.m_val = std::clamp(_x, m_AxisX.m_min, m_AxisX.m_max);
		m_AxisY.m_val = std::clamp(_y, m_AxisY.m_min, m_AxisY.m_max);
	}

	void BlendClip::AddAnimClip(Vector2 _coord, AnimClip* _pClip)
	{
		m_placements.push_back({ _coord, _pClip });
		m_duration = max(_pClip->duration, m_duration);
	}

	bool BlendClip::ComputeWeight(int& _outMatchedIdx)
	{
		// 가중치 초기화
		for (Placement& p : m_placements)
			p.m_weight = 0.0f; 

		// 거리 기준으로 가중치 계산
		Vector2 curCoord(m_AxisX.m_val, m_AxisY.m_val);
		float sum = 0.0f;
		for (int i = 0; i < m_placements.size(); ++i)
		{
			// 현재 입력점과 배치한 클립 좌표 거리 제곱
			const float sqrtDist = Vector2::DistanceSquared(curCoord, m_placements[i].m_coord);
			if (sqrtDist < 1e-5f) // 0에 매우 근사한 값인 경우
			{
				_outMatchedIdx = i;
				return true; // 하나만 고르면 된다고 보냄
			}

			const float w = 1.0f / sqrtDist; // sqrtDist가 작아질수록(가까울수록)  w가 높아짐
			m_placements[i].m_weight = w;
			sum += w;
		}

		if (sum > 0.0f)
		{
			for (Placement& p : m_placements)
				p.m_weight /= sum;
		}

		return false; // 정확히 일치하는 것이 없음
	}
}