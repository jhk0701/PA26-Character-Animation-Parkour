#pragma once
#include "Asset/AnimClip.h"

namespace MiniEngine 
{
	class SkeletalMeshComponent;
	class Animator
	{
	public:
		Animator() {};
		Animator(std::shared_ptr<SkeletalMeshComponent> _meshComp) : m_meshComp(_meshComp) {};
		~Animator() {};

		void Update(float _dt);

	private:
		std::weak_ptr<SkeletalMeshComponent> m_meshComp;
		
		float m_playTime = 0.0f;

		LocalPoseTRS m_poseTarget; // 블렌드, 트랜지션 등 연산이 반영되는 본 위계구조
		// Base Layer : 로코모션

	};
}