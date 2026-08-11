#pragma once
#include "Core/EngineConfig.h"

namespace Content::Config
{
	enum class ETagAct : uint8_t
	{
		Landing, // 착지
		Jump,
		JumpFront,
		JumpFromWall, // 점프에서 탈출
		FallingToLand,

		Vault, // 장애물을 넘고 떨어져야함
		VaultLow = Vault, // 뛰어 넘기
		VaultMid,
		VaultHigh,
		VaultAirToAttach, // 공중에서 잡는 경우

		Mantle, // 높은 장애물, 기어 올라가야하는 경우
		MantleLow = Mantle, // 오르기
		MantleMid,
		MantleHigh,
		MantleAirToAttach, // 공중에서 잡는 경우

		// Wall 벽면 매달리기
		Wall,
		Wall_IdleToHang = Wall,		// 매달리기 시작 : 일반적인 idle -> Hanging
		Wall_RunToHang,				// 매달리기 시작 : 테스트 : 뛰다가 벽에 매달리는 경우
		Wall_AirToHang,				// 매달리기 시작 : 낙하 중 매달리는 경우
		Wall_HangToIdle,		// 매달리기에서 내려옴
		Wall_HangToMantle,			// 매달린 상태에서 꼭대기에 오름
		Wall_HangToMantleOnObs,		// 매달린 상태에서 중간에 있는 장애물에 오름
		Wall_HangToMantleOnSide,	// 매달린 상태에서 측면 장애물로 이동
		Wall_HangToJump,			// 매달린 상태에서 점프

		Wall_HangingMoveUp,
		Wall_HangingMoveDown,
		Wall_HangingMoveLeft,
		Wall_HangingMoveRight,

		Wall_InnerRotateRight,  // 270도 단일 벽의 모서리 돌기 오른쪽
		Wall_InnerRotateLeft,	// 270도 단일 벽의 모서리 돌기 왼쪽
		Wall_OuterRotateRight,	// 90도 벽과 벽이 만나는 지점 오른쪽
		Wall_OuterRotateLeft,	// 90도 벽과 벽이 만나는 지점 왼쪽

		BeamStand,
		Beam_IdleToStand = BeamStand,
		Beam_StandToVault,
		Beam_StandToMantle,

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

		Protrude_HangingMoveUp,
		Protrude_HangingMoveDown,
		Protrude_HangingMoveLeft,
		Protrude_HangingMoveRight,

		Protrude_InAirMoveToPoint,
		Protrude_LandingMoveToPoint,

		Pole_IdleToHang,
		Pole_AirToHang,
		Pole_HangingMoveUp,
		Pole_HangingMoveDown,
		Pole_HangingMoveLeft,
		Pole_HangingMoveRight,

		Test,
		End
	};

	enum class EActionPriority : uint8_t
	{
		Default,
		Override
	};

	bool TryParseTagAct(const std::string& _name, uint8_t& _outTag);
	const char* GetTagActName(uint8_t _tag);
}