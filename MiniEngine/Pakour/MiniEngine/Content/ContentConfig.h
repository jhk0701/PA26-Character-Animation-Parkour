#pragma once

namespace Content::Config 
{
	inline constexpr uint8_t TAG_TYPE_ENV			= 0;
	inline constexpr uint8_t TAG_TYPE_ENV_DETAIL	= 1;

	enum class ETagEnv : uint8_t
	{
		Land,
		Obstacle,

		End
	};

	enum class ETagEnvDetail : uint8_t 
	{
		Default,	// 기본적인 일반지형
		Beam,		// 발판, 봉과 같이 변의 한쪽이 좁고 긴 경우

		End
	};

	enum class ETagAct : uint8_t
	{
		Landing, // 착지
		Jump,
		JumpFromWall, // 점프에서 탈출
		FallingToLand,

		Vault, // 장애물을 넘고 떨어져야함
		VaultLow = Vault, // 뛰어 넘기
		VaultMid,
		VaultHigh,

		Hurdle, // 뛰어 넘고, 계속 달림 (장애물 너머가 평지인 경우)
		HurdleLow = Hurdle,
		HurdleMid,
		HurdleHigh,

		Mantle, // 높은 장애물, 기어 올라가야하는 경우
		MantleLow = Mantle, // 오르기
		MantleMid,
		MantleHigh,

		// Wall		벽면 매달리기
		// Bar		봉 매달리기
		Wall_IdleToHang,		// 매달리기 시작
		Wall_HangToIdle,		// 매달리기에서 내려옴
		Wall_HangToMantle,	// 매달린 상태에서 꼭대기에 오름
		Wall_HangToJump,		// 매달린 상태에서 점프

		Wall_HangingMoveUp,
		Wall_HangingMoveDown,
		Wall_HangingMoveLeft,
		Wall_HangingMoveRight,

		Wall_InnerRotateRight,  // 270도 단일 벽의 모서리 돌기 오른쪽
		Wall_InnerRotateLeft,	// 270도 단일 벽의 모서리 돌기 왼쪽
		Wall_OuterRotateRight,	// 90도 벽과 벽이 만나는 지점 오른쪽
		Wall_OuterRotateLeft,	// 90도 벽과 벽이 만나는 지점 왼쪽

		Beam_Step,

		Beam_IdleToHang,
		Beam_HangToIdle,
		Beam_HangToJump,
		Beam_HangingMoveUp,
		Beam_HangingMoveDown,
		Beam_HangingMoveLeft,
		Beam_HangingMoveRight,

		Test,
		End
	};

	enum class EActionPriority : uint8_t 
	{
		Default,
		Override
	};
}