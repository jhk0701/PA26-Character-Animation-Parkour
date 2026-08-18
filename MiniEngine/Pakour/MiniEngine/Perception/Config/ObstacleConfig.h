#pragma once

namespace MiniEngine 
{
	inline constexpr uint8_t TAG_ENV_DETAIL		= 0;
	inline constexpr uint8_t TAG_SUB_INFO		= 1;
	inline constexpr uint8_t TAG_PRIORITY		= 2;

	enum class ETagEnvDetail : uint8_t
	{
		// 기본적인 일반지형
		// 이 지형에 대해서 레이캐스트를 사용한 높이, 깊이를 측정함
		Default,	

		// 발판, 봉과 같이 변의 한쪽이 좁고 긴 지형
		// sub info로 ETagAxis::X, Z를 사용
		Beam,		
		
		// 벽면 등의 돌출부
		Protrude,	

		// 얇은 기둥, 파이프, 둘레가 좁은 나무 등을 오르는 장애물 유형
		// sub info로 ETagAxis::X, Z를 사용하면, 해당 축 방향으로만 매달림. 
		// 없으면 최초 히트된 노멀에 대해 매달림
		Pole,		

		// 필요 시, 연출을 위한 용도. 
		// sub info에 원하는 액션을 기입
		Direct,		

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
