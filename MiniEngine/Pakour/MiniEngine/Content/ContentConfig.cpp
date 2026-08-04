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
			{ "Landing",					ETagAct::Landing },
			{ "Jump",						ETagAct::Jump },
			{ "JumpFront",					ETagAct::JumpFront },
			{ "JumpFromWall",				ETagAct::JumpFromWall },
			{ "FallingToLand",				ETagAct::FallingToLand },

			{ "VaultLow",					ETagAct::VaultLow },
			{ "Vault",						ETagAct::Vault },
			{ "VaultMid",					ETagAct::VaultMid },
			{ "VaultHigh",					ETagAct::VaultHigh },
			{ "VaultAirToAttach",			ETagAct::VaultAirToAttach },

			{ "MantleLow",					ETagAct::MantleLow },
			{ "Mantle",						ETagAct::Mantle },
			{ "MantleMid",					ETagAct::MantleMid },
			{ "MantleHigh",					ETagAct::MantleHigh },
			{ "MantleAirToAttach",			ETagAct::MantleAirToAttach },

			{ "Wall_IdleToHang",			ETagAct::Wall_IdleToHang },
			{ "Wall",						ETagAct::Wall },
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

			{ "Wall_ProtrudeMoveUp",		ETagAct::Wall_ProtrudeMoveUp },

			{ "Wall_InnerRotateRight",		ETagAct::Wall_InnerRotateRight },
			{ "Wall_InnerRotateLeft",		ETagAct::Wall_InnerRotateLeft },
			{ "Wall_OuterRotateRight",		ETagAct::Wall_OuterRotateRight },
			{ "Wall_OuterRotateLeft",		ETagAct::Wall_OuterRotateLeft },

			{ "Beam_IdleToStand",			ETagAct::Beam_IdleToStand },
			{ "BeamStand",					ETagAct::BeamStand },
			{ "Beam_StandToIdle",			ETagAct::Beam_StandToIdle },
			{ "Beam_StandRotateLeft",		ETagAct::Beam_StandRotateLeft },
			{ "Beam_StandRotateRight",		ETagAct::Beam_StandRotateRight },
			{ "Beam_StandMoveDown",			ETagAct::Beam_StandMoveDown },

			{ "Beam_IdleToHang",			ETagAct::Beam_IdleToHang },
			{ "BeamHanging",				ETagAct::BeamHanging },
			{ "Beam_HangToIdle",			ETagAct::Beam_HangToIdle },
			{ "Beam_HangToJump",			ETagAct::Beam_HangToJump },
			{ "Beam_HangingMoveUp",			ETagAct::Beam_HangingMoveUp },
			{ "Beam_HangingMoveDown",		ETagAct::Beam_HangingMoveDown },
			{ "Beam_HangingMoveLeft",		ETagAct::Beam_HangingMoveLeft },
			{ "Beam_HangingMoveRight",		ETagAct::Beam_HangingMoveRight },

			{ "Test",						ETagAct::Test },
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


		struct TagEnvDetailName
		{
			const char* Name;
			ETagEnvDetail Tag;
		};

		// ETagAct 매핑
		constexpr TagEnvDetailName TAG_ENV_DETAIL_NAMES[] =
		{
			{ "Default",	ETagEnvDetail::Default },
			{ "Beam",		ETagEnvDetail::Beam },
			{ "Protrude",	ETagEnvDetail::Protrude },
		};

		// 추가 검사
		constexpr bool IsEveryTagEnvDetailNamed()
		{
			for (uint8_t value = 0; value < (uint8_t)ETagEnvDetail::End; ++value)
			{
				bool bFound = false;
				for (const TagEnvDetailName& ENTRY : TAG_ENV_DETAIL_NAMES)
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

		static_assert(IsEveryTagEnvDetailNamed(), "TAG_ENV_DETAIL_NAMES 가 ETagEnvDetail와 불일치");
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

	bool TryParseTagEnvDetail(const std::string& _name, uint8_t& _outTag)
	{
		for (const TagEnvDetailName& ENTRY : TAG_ENV_DETAIL_NAMES)
		{
			if (_name == ENTRY.Name)
			{
				_outTag = (uint8_t)ENTRY.Tag;
				return true;
			}
		}

		return false;
	}

	const char* GetTagEnvDetailName(uint8_t _tag)
	{
		for (const TagEnvDetailName& ENTRY : TAG_ENV_DETAIL_NAMES)
		{
			if ((uint8_t)ENTRY.Tag == _tag)
				return ENTRY.Name;
		}

		return "<invalid>";
	}
};
