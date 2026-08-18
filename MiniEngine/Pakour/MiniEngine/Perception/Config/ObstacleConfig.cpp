#include "pch.h"
#include "Perception/Config/ObstacleConfig.h"

namespace MiniEngine 
{
	namespace
	{
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
			{ "Pole",		ETagEnvDetail::Pole },

			{ "Direct",		ETagEnvDetail::Direct },
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
}