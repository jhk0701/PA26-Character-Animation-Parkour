#include "pch.h"
#include "Asset/BlendClip.h"

namespace MiniEngine
{
	// 0인 경우 방지
	constexpr float kEps = 1e-4f;

	BlendClip::BlendClip(int _reserveCnt)
	{
		m_placements.reserve(_reserveCnt);
		m_playTime = 0.0f;
	}

	void BlendClip::Sample(float _dt, const Skeleton& _skeleton, LocalPoseTRS& _outPose)
	{
		if (m_placements.empty())
			return;

		bool bIsUnique = ComputeWeight();
		
		// 여러 모션들 시간 동기화
		m_playTime += _dt;

		bool bIsFirst = true;
		float accW = 0.0f;

		for (const Placement& p : m_placements)
		{
			if (p.m_weight <= 0.0f)
				continue;

			// 이 클립의 현재 시간대의 애니메이션 본 트랜스폼 반영
			p.m_pClip->SampleTRS(m_playTime, _skeleton, m_poseScratch); // 임시 포즈에 현재 클립 원본 자세 보관

			if (bIsUnique && p.m_weight >= 1.0f)
			{
				m_blendedPose = m_poseScratch;
				break;
			}

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

	bool BlendClip::ComputeWeight()
	{
		// 질량중심 좌표계 기준
		Vector2 curCoord(m_AxisX.m_val, m_AxisY.m_val);

		float sum = 0.0f;
		for (Placement& p : m_placements)
		{
			const float sqrtDist = Vector2::DistanceSquared(curCoord, p.m_coord);
			if (sqrtDist < kEps)
			{
				p.m_weight = 1.0f;
				return false; // 하나만 고르면 된다는 의미
			}

			const float w = 1.0f / sqrtDist;
			p.m_weight = w;
			sum += w;
		}

		if (sum > 0.0f)
		{
			for (Placement& p : m_placements)
				p.m_weight /= sum;
		}

		return true;
	}
}