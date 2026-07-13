#pragma once

namespace MiniEngine
{
	class IAnimNotify
	{
	public:
		virtual void Init() = 0;
		virtual void Update(float _dt) = 0;
	};
}
