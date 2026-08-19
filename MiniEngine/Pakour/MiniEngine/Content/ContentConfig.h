#pragma once
#include "Perception/Config/TagConfig.h"

namespace Content::Config
{
	enum class ETagAct : uint8_t
	{
		Reserved_Direct = (uint8_t)MiniEngine::ETagActReserve::Direct_Reserve,

		Landing = (uint8_t)MiniEngine::ETagActReserve::CONTINUE,	// 착지
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

		Wall_IdleToHang_Cliff_MoveDown, // 매달리기 시작 : 절벽에서 내려옴

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

		Protrude_MoveUp,
		Protrude_MoveDown,
		Protrude_MoveLeft,
		Protrude_MoveRight,

		Protrude_InAirMoveToPoint,
		Protrude_LandingMoveToPoint,

		Protrude_ToHangingMoveUp,
		Protrude_ToHangingMoveDown,
		Protrude_ToHangingMoveLeft,
		Protrude_ToHangingMoveRight,

		Pole_IdleToHang,
		Pole_AirToHang,
		Pole_HangingMoveUp,
		Pole_HangingMoveDown,

		Pole_MoveUp_ToOther,
		Pole_MoveDown_ToOther,
		Pole_MoveLeft_ToOther,
		Pole_MoveRight_ToOther,

		Pole_ToHanging_Up,
		Pole_ToHanging_Down,
		Pole_ToHanging_Left,
		Pole_ToHanging_Right,

		Direct_Vault_UnderBar,

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