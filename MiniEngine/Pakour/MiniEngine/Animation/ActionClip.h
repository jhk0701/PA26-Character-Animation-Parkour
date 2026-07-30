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
			bool bApplyRootMotion{ true };

			bool bApplyTranslationX{ true };
			bool bApplyTranslationY{ true };
			bool bApplyTranslationZ{ true };
		};

		ActionClip();
		~ActionClip() { m_clip = nullptr; }

		void Play();
		void Stop();

		void Sample(float _dt, const Skeleton& _skeleton, LocalPoseTRS& _outPose, AnimNotifyParam& _notifyParam);
		float GetDuration() const { return m_duration; }
		AnimClip* GetClip() const { return m_clip; }
		const float GetTickPerSec() const;
		
		void AddClip(AnimClip* _clip);
		void AddNotify(std::shared_ptr<IAnimNotify> _notify);

		void SetApplyRootBone(bool _bApply) { m_rootMotionConfig = { _bApply, _bApply, _bApply, _bApply }; }
		void SetApplyRootBone(RootMotionConfig _config) { m_rootMotionConfig = _config; }
		bool IsApplyingRootBone() const { return m_rootMotionConfig.bApplyRootMotion; }

		const RootMotionConfig& GetRootMotionConfig() const { return m_rootMotionConfig; }

	private:
		bool m_bIsPlaying{ false };
		RootMotionConfig m_rootMotionConfig;

		float m_playTime{ 0.0f };
		float m_duration{ 0.0f };

		AnimClip* m_clip{ nullptr };
		std::vector<std::shared_ptr<IAnimNotify>> m_vecNotify;
	};
}