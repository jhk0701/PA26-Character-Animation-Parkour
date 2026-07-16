#pragma once
#include "Asset/AnimClip.h"

namespace MiniEngine
{
	// 피드백 반영
	// 언리얼 방식 들로네 삼각분할 + 질량중심좌표(barycentric)
	// 로코모션 재생용 (loop)
	class BlendClip
	{
		struct Axis
		{
			float m_val{ 0.0f };
			float m_max{ 1.0f };
			float m_min{ -1.0f };
		};

		struct Placement 
		{
			// 배치 좌표
			Vector2 m_coord{ 0.0f, 0.0f };
			// 재생할 클립
			AnimClip* m_pClip{ nullptr };

			float m_weight{0.0f}; // 재생 가중치 0 ~ 1값
		};

	public:
		BlendClip();
		BlendClip(int _reserveCnt);
		~BlendClip() { m_placements.clear(); }

		// 현재 입력된 좌표 축을 가지고 애니메이션 가중치 계산
		void Sample(float _dt, const Skeleton& _skeleton, LocalPoseTRS& _outPose);
		// float GetDuration() const { return m_duration; }

		bool ComputeWeight(int& _outMatchedIdx);

		Axis& GetAxisX() { return m_AxisX; }
		Axis& GetAxisY() { return m_AxisY; }

		void SetAxisValue(const Vector2& _vec);
		void AddAnimClip(Vector2 _coord, AnimClip* _pClip);

	private:
		struct Tri 
		{ 
			int i0 = -1, i1 = -1, i2 = -1; 
		};
		// 삼각분할 결과
		enum class EFallback 
		{ 
			Triangulated, Empty, Single, Segment 
		}; 

		float Cross(const Vector2& _a, const Vector2& _b) { return _a.Cross(_b).x; }

		// 삼각형 abc가 점 p를 포함하는지
		// Bowyer-Watson의 bad triangle 판정
		bool InCircumcircle(const Vector2& _a, const Vector2& _b, const Vector2& _c, const Vector2& _p) const;
		
		// 점 p에서 가장 가까운 삼각형에 대한 질량들의 가중치값 반환
		void ClosestPointBary(const Vector2& _a, const Vector2& _b, const Vector2& _c, const Vector2& _p,
			float& _u, float& _v, float& _w);

		void EnsureTriangluated();

		std::vector<Tri> m_tris;
		EFallback m_fallback = EFallback::Empty;
		bool m_triDirty = true; // AddAnimClip 호출 시, tri 재구성 필요

		Axis m_AxisX;
		Axis m_AxisY;
		LocalPoseTRS m_blendedPose;	// 실제로 전달할 포즈
		LocalPoseTRS m_poseScratch; // 임시 보관용

		float m_playTime{ 0.0f };

		// x, y축 특정 위치에 애니메이션 클립 배치
		std::vector<Placement> m_placements;
	};
}