#include "pch.h"
#include "Content/ContentConfig.h"

namespace Content::Config
{
	namespace
	{
		struct TagActName
		{
			const char* Name;
			ETagAct Tag;
		};

		// ETagAct 매핑
		constexpr TagActName TAG_ACT_NAMES[] =
		{
			{ "Reserved_Direct",			ETagAct::Reserved_Direct },

			{ "Landing",					ETagAct::Landing },

			{ "Jump",						ETagAct::Jump },
			{ "JumpFront",					ETagAct::JumpFront },
			{ "JumpFromWall",				ETagAct::JumpFromWall },

			{ "FallingToLand",				ETagAct::FallingToLand },
			{ "FallingToLandFront",			ETagAct::FallingToLandFront },
			{ "FallingToLandRoll",			ETagAct::FallingToLandRoll },

			{ "VaultLow",					ETagAct::VaultLow },
			{ "Vault",						ETagAct::Vault },
			{ "VaultMid",					ETagAct::VaultMid },
			{ "VaultHigh",					ETagAct::VaultHigh },
			{ "VaultAirToAttach",			ETagAct::VaultAirToAttach },
			{ "VaultDeep",					ETagAct::VaultDeep },

			{ "MantleLow",					ETagAct::MantleLow },
			{ "Mantle",						ETagAct::Mantle },
			{ "MantleMid",					ETagAct::MantleMid },
			{ "MantleHigh",					ETagAct::MantleHigh },
			{ "MantleAirToAttach",			ETagAct::MantleAirToAttach },

			{ "Wall_IdleToHang",			ETagAct::Wall_IdleToHang },
			{ "Wall",						ETagAct::Wall },
			{ "Wall_RunToHang",				ETagAct::Wall_RunToHang },
			{ "Wall_AirToHang",				ETagAct::Wall_AirToHang },
			{ "Wall_HangToIdle",			ETagAct::Wall_HangToIdle },
			{ "Wall_HangToMantle",			ETagAct::Wall_HangToMantle },
			{ "Wall_HangToMantleOnObs",		ETagAct::Wall_HangToMantleOnObs },
			{ "Wall_HangToMantleOnSide",	ETagAct::Wall_HangToMantleOnSide },
			{ "Wall_HangToJump",			ETagAct::Wall_HangToJump },

			{ "Wall_HangingMoveUp",			ETagAct::Wall_HangingMoveUp },
			{ "Wall_HangingMoveDown",		ETagAct::Wall_HangingMoveDown },
			{ "Wall_HangingMoveLeft",		ETagAct::Wall_HangingMoveLeft },
			{ "Wall_HangingMoveRight",		ETagAct::Wall_HangingMoveRight },

			{ "Wall_IdleToHang_Cliff_MoveDown",		ETagAct::Wall_IdleToHang_Cliff_MoveDown },

			{ "Beam_IdleToStand",			ETagAct::Beam_IdleToStand },
			{ "BeamStand",					ETagAct::BeamStand },
			{ "Beam_StandToIdle",			ETagAct::Beam_StandToIdle },
			{ "Beam_StandRotateLeft",		ETagAct::Beam_StandRotateLeft },
			{ "Beam_StandRotateRight",		ETagAct::Beam_StandRotateRight },
			{ "Beam_StandMoveDown",			ETagAct::Beam_StandMoveDown },

			{ "Beam_StandToVault",			ETagAct::Beam_StandToVault },
			{ "Beam_StandToMantle",			ETagAct::Beam_StandToMantle },
			{ "Beam_HangingJump",			ETagAct::Beam_HangingJump },

			{ "Beam_IdleToHang",			ETagAct::Beam_IdleToHang },
			{ "BeamHanging",				ETagAct::BeamHanging },
			{ "Beam_HangToIdle",			ETagAct::Beam_HangToIdle },
			{ "Beam_HangToJump",			ETagAct::Beam_HangToJump },
			{ "Beam_HangingMoveUp",			ETagAct::Beam_HangingMoveUp },
			{ "Beam_HangingMoveDown",		ETagAct::Beam_HangingMoveDown },
			{ "Beam_HangingMoveLeft",		ETagAct::Beam_HangingMoveLeft },
			{ "Beam_HangingMoveRight",		ETagAct::Beam_HangingMoveRight },

			{ "Protrude_MoveUp",			ETagAct::Protrude_MoveUp },
			{ "Protrude_MoveDown",			ETagAct::Protrude_MoveDown },
			{ "Protrude_MoveLeft",			ETagAct::Protrude_MoveLeft },
			{ "Protrude_MoveRight",			ETagAct::Protrude_MoveRight },

			{ "Protrude_InAirMoveToPoint",		ETagAct::Protrude_InAirMoveToPoint },
			{ "Protrude_LandingMoveToPoint",	ETagAct::Protrude_LandingMoveToPoint },

			{ "Protrude_ToHangingMoveUp",		ETagAct::Protrude_ToHangingMoveUp },
			{ "Protrude_ToHangingMoveDown",		ETagAct::Protrude_ToHangingMoveDown },
			{ "Protrude_ToHangingMoveLeft",		ETagAct::Protrude_ToHangingMoveLeft },
			{ "Protrude_ToHangingMoveRight",	ETagAct::Protrude_ToHangingMoveRight },

			{ "Pole_IdleToHang",				ETagAct::Pole_IdleToHang },
			{ "Pole_AirToHang",					ETagAct::Pole_AirToHang },
			{ "Pole_HangingMoveUp",				ETagAct::Pole_HangingMoveUp },
			{ "Pole_HangingMoveDown",			ETagAct::Pole_HangingMoveDown },

			{ "Pole_MoveUp_ToOther",			ETagAct::Pole_MoveUp_ToOther },
			{ "Pole_MoveDown_ToOther",			ETagAct::Pole_MoveDown_ToOther },
			{ "Pole_MoveLeft_ToOther",			ETagAct::Pole_MoveLeft_ToOther },
			{ "Pole_MoveRight_ToOther",			ETagAct::Pole_MoveRight_ToOther },

			{ "Pole_ToHanging_Up",				ETagAct::Pole_ToHanging_Up },
			{ "Pole_ToHanging_Down",			ETagAct::Pole_ToHanging_Down },
			{ "Pole_ToHanging_Left",			ETagAct::Pole_ToHanging_Left },
			{ "Pole_ToHanging_Right",			ETagAct::Pole_ToHanging_Right },

			{ "Direct_Vault_UnderBar",			ETagAct::Direct_Vault_UnderBar },
			{ "Direct_Run_Sliding",				ETagAct::Direct_Run_Sliding },
		};

		// ETagAct 추가 시, 값 검사
		constexpr bool IsEveryTagActNamed()
		{
			for (uint8_t value = 0; value < (uint8_t)ETagAct::End; ++value)
			{
				bool bFound = false;
				for (const TagActName& ENTRY : TAG_ACT_NAMES)
				{
					if ((uint8_t)ENTRY.Tag == value)
					{
						bFound = true;
						break;
					}
				}

				if (bFound == false)
					return false;
			}

			return true;
		}

		static_assert(IsEveryTagActNamed(), "TAG_ACT_NAMES 가 ETagAct 와 불일치");

	}

	bool TryParseTagAct(const std::string& _name, uint8_t& _outTag)
	{
		for (const TagActName& ENTRY : TAG_ACT_NAMES)
		{
			if (_name == ENTRY.Name)
			{
				_outTag = (uint8_t)ENTRY.Tag;
				return true;
			}
		}

		return false;
	}

	const char* GetTagActName(uint8_t _tag)
	{
		for (const TagActName& ENTRY : TAG_ACT_NAMES)
		{
			if ((uint8_t)ENTRY.Tag == _tag)
				return ENTRY.Name;
		}

		return "<invalid>";
	}

};
