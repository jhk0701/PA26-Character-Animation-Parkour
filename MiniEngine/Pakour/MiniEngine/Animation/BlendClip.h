#pragma once

#include "Animation/IAnimatorClip.h"

namespace MiniEngine
{
	// 로코모션 재생용 (loop)
	class BlendClip : public IAnimatorClip
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
			Vector2 m_coord{0.0f, 0.0f};
			// 재생할 클립
			AnimClip* m_pClip{ nullptr };

			float m_weight{0.0f}; // 재생 가중치 0 ~ 1값
		};

	public:
		BlendClip();
		BlendClip(int _reserveCnt);
		~BlendClip() { m_placements.clear(); }

		// 현재 입력된 좌표 축을 가지고 애니메이션 가중치 계산
		void Sample(float _dt, const Skeleton& _skeleton, LocalPoseTRS& _outPose) override;
		float GetDuration() const override { return m_duration; }
		AnimClip* GetClip() const override { return nullptr; }

		bool ComputeWeight(int& _outMatchedIdx);

		Axis& GetAxisX() { return m_AxisX; }
		Axis& GetAxisY() { return m_AxisY; }

		void SetAxisValue(const Vector2& _vec);
		void AddAnimClip(Vector2 _coord, AnimClip* _pClip);

	private:
		Axis m_AxisX;
		Axis m_AxisY;
		LocalPoseTRS m_blendedPose;	// 실제로 전달할 포즈
		LocalPoseTRS m_poseScratch; // 임시 보관용

		float m_playTime{ 0.0f };
		float m_duration{ 0.0f }; // 클립들 중 제일 긴 것을 보관

		// x, y축 특정 위치에 애니메이션 클립 배치
		std::vector<Placement> m_placements;
	};
}