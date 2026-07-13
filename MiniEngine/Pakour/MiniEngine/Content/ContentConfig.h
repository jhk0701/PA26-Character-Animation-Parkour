#pragma once

namespace Content::Config 
{
	inline constexpr uint8_t TAG_TYPE_ENV		= 0;
	inline constexpr uint8_t TAG_TYPE_ACT		= 1;
	inline constexpr uint8_t TAG_TYPE_HEIGHT	= 2;

	enum class ETagEnv : uint8_t
	{
		Land,
		Obstacle,

		End
	};

	enum class ETagAct : uint8_t
	{
		Landing,
		Jump,
		Vault, // 담넘기 valut
		Mantle,
		Hurdle,

		End
	};
}