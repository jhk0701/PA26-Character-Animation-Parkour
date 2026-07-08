#pragma once

#include "Asset/AnimClip.h"
#include "Asset/Skeleton.h"

namespace MiniEngine
{
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
			// 좌표
			Vector2 m_coord{0.0f, 0.0f};
			// 재생할 클립
			AnimClip* m_pClip{ nullptr };

			float m_weight{0.0f}; // 재생 가중치 0 ~ 1값
		};

	public:
		BlendClip() {};
		BlendClip(int _reserveCnt);
		~BlendClip() {};

		// 현재 입력된 좌표 축을 가지고 애니메이션 가중치 계산
		void Sample(float _dt, Skeleton& _skeleton, LocalPoseTRS& _outPose);
		bool ComputeWeight(); // 질량중심좌표계 이용, 각 클립들의 가중치 계산

		Axis& GetAxisX() { return m_AxisX; }
		Axis& GetAxisY() { return m_AxisY; }
		std::vector<Placement>& GetPlaceholder() { return m_placements; };

		void SetAxisValue(float _x, float _y);
		void AddAnimClip(Vector2 _coord, AnimClip* _pClip) { m_placements.push_back({ _coord, _pClip}); };

	private:
		Axis m_AxisX;
		Axis m_AxisY;
		LocalPoseTRS m_blendedPose;
		LocalPoseTRS m_poseScratch;

		float m_blendPhase{ 0.0f };

		// x, y축 특정 위치에 애니메이션 클립 배치
		std::vector<Placement> m_placements;
	};
}