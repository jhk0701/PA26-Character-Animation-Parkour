#pragma once

namespace MiniEngine
{
	class Actor;
	struct AnimNotifyParam 
	{
		Actor* m_pActor;
	};

	class IAnimNotify
	{
	public:
		virtual void Init() = 0;
		virtual void Update(float _dt, AnimNotifyParam& _param) = 0;
	};
}
