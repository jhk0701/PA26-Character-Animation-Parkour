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
		FallingToLand,
		
		Vault,
		Vault_Low = Vault, // 뛰어 넘고, 절벽으로 떨어짐
		Vault_Mid,
		Vault_High, 

		Hurdle, // 뛰어 넘고, 계속 달림

		Mantle,
		Mantle_Low = Mantle, // 오르기
		Mantle_Mid,
		Mantle_High,

		End
	};
}