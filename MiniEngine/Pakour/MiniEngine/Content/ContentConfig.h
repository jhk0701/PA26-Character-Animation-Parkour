#pragma once

namespace Content::Config 
{
	extern constexpr uint8_t TAG_TYPE_ENV		= 0;
	extern constexpr uint8_t TAG_TYPE_ACT		= 1;

	enum class ETagEnv : uint8_t
	{
		Land,
		Obstacle,

		End
	};

	enum class ETagAct : uint8_t
	{
		Landing,
		JumpOver, // ´ã³Ñ±â valut

		End
	};
}