#pragma once
#include "Asset/AnimClip.h"
#include "Animation/IAnimNotify.h"
#include <functional>

namespace MiniEngine
{
	// 단발 재생용
	class ActionClip
	{
	public:
		struct RootMotionConfig
		{
			RootMotionConfig() {}
			RootMotionConfig(bool _b) : 
				bApplyRootMotion(_b),
				bApplyTranslationX(_b),
				bApplyTranslationY(_b),
				bApplyTranslationZ(_b),
				bApplyRotationYaw(_b)
			{}

			bool bApplyRootMotion{ true };
			bool bApplyTranslationX{ true };
			bool bApplyTranslationY{ true };
			bool bApplyTranslationZ{ true };
			bool bApplyRotationYaw{ true };
		};

		ActionClip();
		~ActionClip() { m_clip = nullptr; }

		void Play();
		void Stop();

		void Sample(float _dt, const Skeleton& _skeleton, LocalPoseTRS& _outPose, AnimNotifyParam& _notifyParam);
		const float GetDuration() const;
		AnimClip* GetClip() const { return m_clip; }
		const float GetTickPerSec() const;
		
		void AddClip(AnimClip* _clip);
		void AddNotify(std::shared_ptr<IAnimNotify> _notify);

		void SetApplyRootBone(bool _bApply) { m_rootMotionConfig = RootMotionConfig(_bApply); }
		void SetApplyRootBone(RootMotionConfig _config) { m_rootMotionConfig = _config; }
		bool IsApplyingRootBone() const { return m_rootMotionConfig.bApplyRootMotion; }

		const RootMotionConfig& GetRootMotionConfig() const { return m_rootMotionConfig; }

		void SetSpeed(const float _spd);
		float GetSpeed() const { return m_speed; }
		
		void SetOffset(const float _startOffset, const float _endOffset);
		const float GetStartTime() const;
		const float GetEndTime() const;

	private:
		bool m_bIsPlaying{ false };
		RootMotionConfig m_rootMotionConfig;

		float m_playTime{ 0.0f };
		float m_duration{ 0.0f };

		float m_speed{ 1.0f }; 
		float m_startOffset{ 0.0f };	// 시작에서 더할 것
		float m_endOffset{ 0.0f };		// 끝에서 뺄 것

		AnimClip* m_clip{ nullptr };
		std::vector<std::shared_ptr<IAnimNotify>> m_vecNotify;
	};
}