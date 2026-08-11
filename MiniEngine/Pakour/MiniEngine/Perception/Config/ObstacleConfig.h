#pragma once

namespace MiniEngine 
{
	inline constexpr uint8_t TAG_ENV_DETAIL = 0;
	inline constexpr uint8_t TAG_SUB_INFO = 1;

	enum class ETagEnvDetail : uint8_t
	{
		Default,	// 기본적인 일반지형
		Beam,		// 발판, 봉과 같이 변의 한쪽이 좁고 긴 경우
		Protrude,	// 벽면 등의 돌출부
		Pole,		// 얇은 기둥, 파이프, 둘레가 좁은 나무 등

		Customize,	// 일부 특수 처리를 위한 용도 -> sub info에 원하는 액션을 기입

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

	bool TryParseTagEnvDetail(const std::string& _name, uint8_t& _outTag);
	const char* GetTagEnvDetailName(uint8_t _tag);
}
