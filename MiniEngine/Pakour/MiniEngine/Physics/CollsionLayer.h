#pragma once

namespace MiniEngine::Physics
{
	// Raycast 씬 쿼리용 비트마스크
	// queryFilterData.word0에 저장
	enum class Layer : uint32_t 
	{
		Default			= 1u << 0, // physicsWorld에서 shape 생성 시 기본 값
		Ground			= 1u << 1,
		Character		= 1u << 2,
		Obstacle		= 1u << 3,
		ObstacleLedge	= 1u << 4,
	};
	
	constexpr uint32_t ToMask(Layer _layer) { return static_cast<uint32_t>(_layer); }
	constexpr uint32_t operator|(Layer _a, Layer _b) { return ToMask(_a) | ToMask(_b); }
	constexpr uint32_t operator|(uint32_t _a, Layer _b) { return _a | ToMask(_b); }
	constexpr uint32_t operator|(Layer _a, uint32_t _b) { return ToMask(_a) | _b; }

	namespace LayerMask
	{
		// PhysicsWorld::Raycast 가 마스크 0 을 히트 없음으로 조기 반환해야함
		// 필터 없음 -> 바로 통과 처리
		constexpr uint32_t NONE = 0u;
		constexpr uint32_t ALL = 0xFFFFFFFFu;
	}
}