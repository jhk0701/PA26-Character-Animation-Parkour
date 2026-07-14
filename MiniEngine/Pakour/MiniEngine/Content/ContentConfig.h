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
		Vault_Low = Vault, // 뛰어 넘기
		Vault_Mid,
		Vault_High, 

		// Hurdle, // 뛰어 넘고, 계속 달림 -> Valut와 차이가 없음
		Mantle,
		Mantle_Low = Mantle, // 오르기
		Mantle_Mid,
		Mantle_High,

		IdleToHang,		// 매달리기 시작
		HangToIdle,		// 매달리기에서 내려옴
		HangToMantle,	// 매달린 상태에서 꼭대기에 오름
		HangToJump,		// 매달린 상태에서 점프

		End
	};
}