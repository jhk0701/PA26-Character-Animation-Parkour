#pragma once
#include "Asset/AnimClip.h"

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
			float m_x{ 0.0f };
			float m_y{ 0.0f };

			// 애니메이션 클립 idx
			int m_idx{ 0 };
		};

	public:
		BlendClip() {};
		~BlendClip() {};

		// 현재 입력된 좌표 축을 가지고 애니메이션 가중치 계산
		void Sample(float _timeSec, LocalPoseTRS& _outPose);
		
		Axis& GetAxisX() { return m_AxisX; }
		Axis& GetAxisY() { return m_AxisY; }
		std::vector<Placement>& GetPlaceholder() { return m_placements; };

		void SetAxis(float _x, float _y) { m_AxisX.m_val = _x; m_AxisY.m_val = _y; };

	private:
		Axis m_AxisX;
		Axis m_AxisY;

		// x, y축 특정 위치에 애니메이션 클립 배치
		std::vector<Placement> m_placements;
	};
}