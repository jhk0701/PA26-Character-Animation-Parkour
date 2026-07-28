#pragma once

namespace Content::Config
{
	inline constexpr uint8_t TAG_ENV		= 0;
	inline constexpr uint8_t TAG_ENV_DETAIL	= 1;
	inline constexpr uint8_t TAG_SUB_INFO	= 2;

	// 일반 장애물 측량 파라미터
	// 측정(PerceptionQueryTree)과 분류(CharacterState)가 같은 값을 봐야 하므로 공유 헤더에 둔다
	inline constexpr float   HEIGHT_PROBE_RADIUS	= 0.5f;	// STEP 과 커플링 — 함께 바꿀 것
	inline constexpr float   HEIGHT_PROBE_STEP		= 1.0f;	// = RADIUS * 2 여야 밴드가 틈/중복 없이 접한다
	inline constexpr uint8_t HEIGHT_PROBE_MAX_BAND	= 3;	// 3.0m 이상은 벽으로 보고 매달린다
	inline constexpr float   HEIGHT_PROBE_FORWARD	= 0.1f;	// 스윕이 eMTD 라 초기 겹침도 히트 — 짧아도 된다
	inline constexpr float   DEPTH_PROBE_STEP		= 0.5f;
	inline constexpr uint8_t DEPTH_PROBE_MAX_STEP	= 2;	// 최대 1.0m 까지만 잰다
	inline constexpr float   DEPTH_PROBE_DOWN_DIST	= 2.0f;
	inline constexpr float   DEPTH_PROBE_LIFT		= 0.05f;// 꼭대기 표면에서 시작하는 퇴화 방지
	inline constexpr float   MIN_MANTLE_DEPTH		= 1.0f;	// 이 값 이상이어야 Mantle, 미만은 Vault

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
		Protrude,	// 벽면 등의 돌출부

		End
	};

	// 특정 액터의 주된 방향
	// Beam 지형물의 경우 어느 축으로 길게 뻗었는지 : 로컬 기준
	enum class ETagAxis : uint8_t 
	{
		X, // Transform에서 Right
		Y, // Transform에서 Up
		Z  // Transform에서 Forward
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

		Mantle, // 높은 장애물, 기어 올라가야하는 경우
		MantleLow = Mantle, // 오르기
		MantleMid,
		MantleHigh,

		// Wall		벽면 매달리기
		Wall,
		Wall_IdleToHang = Wall,		// 매달리기 시작
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

		// 다른 지형으로 뛰어다닐 것
		Protrude,
		Protrude_IdleToHang = Protrude,
		Protrude_HangToIdle,
		Protrude_HangToMantle,	// 매달린 상태에서 꼭대기에 오름
		Protrude_HangToJump,	// 매달린 상태에서 점프 (탈출)
		
		Protrude_JumpUp,
		Protrude_JumpDown,
		Protrude_JumpLeft,
		Protrude_JumpRight,

		BeamStand,
		Beam_IdleToStand = BeamStand,
		Beam_StandToIdle,
		Beam_StandRotateLeft,
		Beam_StandRotateRight,
		Beam_StandMoveDown,

		BeamHanging,
		Beam_IdleToHang = BeamHanging,
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