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

	void BlendClip::SetAxisValue(const Vector2& _vec)
	{
		m_AxisX.m_val = std::clamp(_vec.x, m_AxisX.m_min, m_AxisX.m_max);
		m_AxisY.m_val = std::clamp(_vec.y, m_AxisY.m_min, m_AxisY.m_max);
	}

	void BlendClip::AddAnimClip(Vector2 _coord, AnimClip* _pClip)
	{
		m_placements.push_back({ _coord, _pClip });
		m_triDirty = true;
	}

	bool BlendClip::InCircumcircle(const Vector2& _a, const Vector2& _b, const Vector2& _c, const Vector2& _p) const
	{
		const float orient = (_b.x - _a.x) * (_c.y - _a.y) - (_b.y - _a.y) * (_c.x - _a.x);
		const float ax = _a.x - _p.x, ay = _a.y - _p.y;
		const float bx = _b.x - _p.x, by = _b.y - _p.y;
		const float cx = _c.x - _p.x, cy = _c.y - _p.y;
		const float a2 = ax * ax + ay * ay;
		const float b2 = bx * bx + by * by;
		const float c2 = cx * cx + cy * cy;
		float det = ax * (by * c2 - b2 * cy) - ay * (bx * c2 - b2 * cx) + a2 * (bx * cy - by * cx);

		if (orient < 0.0f) 
			det = -det; 

		return det > 1e-7f;
	}

	void BlendClip::ClosestPointBary(const Vector2& _a, const Vector2& _b, const Vector2& _c, const Vector2& _p, float& _u, float& _v, float& _w)
	{
		const Vector2 ab = _b - _a, ac = _c - _a, ap = _p - _a;
		const float d1 = ab.Dot(ap), d2 = ac.Dot(ap);

		if (d1 <= 0.0f && d2 <= 0.0f) // 정점 a에 맞는 경우
		{ 
			_u = 1.0f; 
			_v = 0.0f; 
			_w = 0.0f; 
			return; 
		} 

		const Vector2 bp = _p - _b;
		const float d3 = ab.Dot(bp), d4 = ac.Dot(bp);
		if (d3 >= 0.0f && d4 <= d3) // 정점 b
		{ 
			_u = 0.0f; 
			_v = 1.0f; 
			_w = 0.0f; 
			return; 
		} 
		
		const float vc = d1 * d4 - d3 * d2;
		if (vc <= 0.0f && d1 >= 0.0f && d3 <= 0.0f) // 변 ab
		{
			const float t = d1 / (d1 - d3);
			_u = 1.0f - t; 
			_v = t; 
			_w = 0.0f; 
			return;
		}

		const Vector2 cp = _p - _c;
		const float d5 = ab.Dot(cp), d6 = ac.Dot(cp);
		if (d6 >= 0.0f && d5 <= d6) // 정점 c
		{ 
			_u = 0.0f;
			_v = 0.0f;
			_w = 1.0f; 
			return; 
		}

		const float vb = d5 * d2 - d1 * d6;
		if (vb <= 0.0f && d2 >= 0.0f && d6 <= 0.0f)                                 // 변 ac
		{
			const float t = d2 / (d2 - d6);
			_u = 1.0f - t; 
			_v = 0.0f; 
			_w = t; 
			return;
		}

		const float va = d3 * d6 - d5 * d4;
		if (va <= 0.0f && (d4 - d3) >= 0.0f && (d5 - d6) >= 0.0f)                    // 변 bc
		{
			const float t = (d4 - d3) / ((d4 - d3) + (d5 - d6));
			_u = 0.0f; 
			_v = 1.0f - t; 
			_w = t; 
			return;
		}

		const float denom = 1.0f / (va + vb + vc);                                  // 내부면
		_v = vb * denom;
		_w = vc * denom;
		_u = 1.0f - _v - _w;
	}

	void BlendClip::EnsureTriangluated()
	{
		if (!m_triDirty)
			return;

		m_triDirty = false;
		m_tris.clear();

		// 삼각형이 불가한 경우 사전 처리
		const int n = static_cast<int>(m_placements.size());
		if (n == 0) 
		{ 
			m_fallback = EFallback::Empty;  
			return; 
		}
		if (n == 1) 
		{ 
			m_fallback = EFallback::Single; 
			return; 
		}

		std::vector<Vector2> pts;
		pts.reserve(n + 3);

		float minx = (std::numeric_limits<float>::max)(), miny = minx;
		float maxx = std::numeric_limits<float>::lowest(), maxy = maxx;
		for (const Placement& p : m_placements)
		{
			pts.push_back(p.m_coord);
			minx = min(minx, p.m_coord.x);
			miny = min(miny, p.m_coord.y);
			maxx = max(maxx, p.m_coord.x);
			maxy = max(maxy, p.m_coord.y);
		}

		float dmax = max(maxx - minx, maxy - miny);
		if (dmax <= 1e-6f) // 모든 샘플이 한 점 — Segment 폴백이 처리
			dmax = 1.0f; 

		const float midx = 0.5f * (minx + maxx);
		const float midy = 0.5f * (miny + maxy);
		pts.emplace_back(midx - 20.0f * dmax, midy - dmax);
		pts.emplace_back(midx, midy + 20.0f * dmax);
		pts.emplace_back(midx + 20.0f * dmax, midy - dmax);

		std::vector<Tri> tris{ { n, n + 1, n + 2 } };

		// 점 삽입
		struct Edge { int a, b; };
		std::vector<Edge> edges;
		for (int i = 0; i < n; ++i)
		{
			const Vector2& p = pts[i];
			edges.clear();

			// 외접원에 p 를 포함하는 삼각형 제거
			// 해당 변을 수집
			for (size_t t = 0; t < tris.size();)
			{
				const Tri& tr = tris[t];
				if (InCircumcircle(pts[tr.i0], pts[tr.i1], pts[tr.i2], p))
				{
					edges.push_back({ tr.i0, tr.i1 });
					edges.push_back({ tr.i1, tr.i2 });
					edges.push_back({ tr.i2, tr.i0 });
					tris[t] = tris.back();
					tris.pop_back();
				}
				else ++t;
			}

			// 두 번 나온 변(공유=cavity 내부)은 제거
			// 한 번만 나온 변(경계)만 남기기
			for (size_t e = 0; e < edges.size(); ++e)
			{
				bool shared = false;
				for (size_t f = 0; f < edges.size(); ++f)
				{
					if (e == f) continue;
					if ((edges[e].a == edges[f].a && edges[e].b == edges[f].b) ||
						(edges[e].a == edges[f].b && edges[e].b == edges[f].a))
					{
						shared = true; break;
					}
				}
				if (!shared)
					tris.push_back({ edges[e].a, edges[e].b, i });
			}
		}

		// super-triangle 정점을 물고 있는 삼각형 제거 → 남은 것이 실제 삼각분할
		for (const Tri& tr : tris)
			if (tr.i0 < n && tr.i1 < n && tr.i2 < n)
				m_tris.push_back({ tr.i0, tr.i1, tr.i2 });

		// 삼각형이 하나도 없다 = 공선/2점 등등
		m_fallback = m_tris.empty() ? EFallback::Segment : EFallback::Triangulated;
	}

	bool BlendClip::ComputeWeight(int& _outMatchedIdx)
	{
		EnsureTriangluated(); // 더티 플래그로 1번만 정렬

		Vector2 curCoord{ m_AxisX.m_val, m_AxisY.m_val };

		// 가중치 초기화
		for (Placement& p : m_placements)
			p.m_weight = 0.0f; 

		const float kWeightEps = 1e-6f;

		switch (m_fallback)
		{
		case BlendClip::EFallback::Empty:
			return false;
		case BlendClip::EFallback::Single:
			{
				_outMatchedIdx = 0;
				m_placements[0].m_weight = 1.0f;

				return true;
			}
		case BlendClip::EFallback::Segment:
			{
				// 공선/2점: 가장 멀리 떨어진 두 샘플로 축을 잡고 그 위에서 1D bracketing.
				int i0 = 0;
				int i1 = 0; 
				float best = -1.0f;
				for (size_t a = 0; a < m_placements.size(); ++a)
					for (size_t b = a + 1; b < m_placements.size(); ++b)
					{
						const float d = Vector2::DistanceSquared(m_placements[a].m_coord, m_placements[b].m_coord);
						if (d > best) 
						{ 
							best = d; 
							i0 = static_cast<int>(a); 
							i1 = static_cast<int>(b); 
	}
					}

				if (best <= 1e-12f)  // 전부 한 점
				{ 
					_outMatchedIdx = 0;
					m_placements[0].m_weight = 1.0f;
					return true;
				} 

				const Vector2 origin = m_placements[i0].m_coord;
				Vector2 dir = m_placements[i1].m_coord - origin;
				dir.Normalize();

				std::vector<std::pair<int, float>> proj;
				proj.reserve(m_placements.size());
				for (int i = 0; i < m_placements.size(); ++i)
					proj.emplace_back(i, (m_placements[i].m_coord - origin).Dot(dir));

				std::sort(proj.begin(), proj.end(),
					[](const auto& _a, const auto& _b) 
					{ 
						return _a.second < _b.second; 
					}
				);

				const float q = (curCoord - origin).Dot(dir);
				if (q <= proj.front().second) 
				{ 
					_outMatchedIdx = proj.front().first;
					m_placements[proj.front().first].m_weight = 1.0f;
					return true; 
				}

				if (q >= proj.back().second) 
				{
					_outMatchedIdx = proj.back().first;
					m_placements[proj.back().first].m_weight = 1.0f;
					return true; 
				}

				for (size_t k = 0; k + 1 < proj.size(); ++k)
					if (q >= proj[k].second && 
						q <= proj[k + 1].second)
					{
						const float span = proj[k + 1].second - proj[k].second;
						const float t = (span > 0.0f) ? (q - proj[k].second) / span : 0.0f;

						if (t < 1.0f)
							m_placements[proj[k].first].m_weight = 1.0f - t;
						if (t > 0.0f) 
							m_placements[proj[k + 1].first].m_weight = t;

						return false;
					}

				return false;
			}
		case BlendClip::EFallback::Triangulated:
			{
				// 최근접점 거리가 최소인 삼각형 고르기
				// curCoord 포함하는 삼각형은 거리 0(정확 barycentric)
				
				float bestD2 = (std::numeric_limits<float>::max)();
				float bu = 1.0f, bv = 0.0f, bw = 0.0f;

				const Tri* bestTri = nullptr;
				for (const Tri& tr : m_tris)
				{
					const Vector2& A = m_placements[tr.i0].m_coord;
					const Vector2& B = m_placements[tr.i1].m_coord;
					const Vector2& C = m_placements[tr.i2].m_coord;
					
					float u, v, w;
					ClosestPointBary(A, B, C, curCoord, u, v, w);

					const Vector2 cp = A * u + B * v + C * w;
					const float d2 = Vector2::DistanceSquared(curCoord, cp);

					if (d2 < bestD2) 
					{ 
						bestD2 = d2; 
						bu = u; 
						bv = v;
						bw = w; 
						bestTri = &tr; 
					}

					if (d2 <= 0.0f) 
						break; // 포함 삼각형 — 더 볼 것 없음.
				}

				if (!bestTri) 
					return false;

				if (bu > kWeightEps)
					m_placements[bestTri->i0].m_weight = bu;
				if (bv > kWeightEps) 
					m_placements[bestTri->i1].m_weight = bv;
				if (bw > kWeightEps) 
					m_placements[bestTri->i2].m_weight = bw;

				return false;
			}
		}

		return false; // 정확히 일치하는 것이 없음
	}
}