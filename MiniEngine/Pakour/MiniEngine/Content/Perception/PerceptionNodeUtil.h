#pragma once
#include "Scene/PerceptionComponent.h"

namespace MiniEngine { class Actor; class IObstacle; }
class Character;

namespace PerceptionNodeUtil 
{
	std::shared_ptr<Character> ToChar(std::shared_ptr<MiniEngine::Actor> _actor);
	MiniEngine::Vector3 GetCharacterCenterPosition(MiniEngine::TravelContext& _context);
	MiniEngine::IObstacle* ToIObstacle(void* _p);

	template<typename THit>
	void FillFromResult(MiniEngine::TravelContext& _context, const THit& _result)
	{
		_context.m_pFirstObstacle = ToIObstacle(_result.GetActor());
		_context.m_firstObstacleHitPos = _result.m_pos;
		_context.m_firstObstacleHitNrm = _result.m_nrm;
		_context.m_distance = _result.m_distance;
		_context.m_ledge = _result.m_pos.y;
	}

	// 캐릭터 기준으로 현재 위치에서 특정 방향에 장애물이 있는지 체크
	bool CheckObstacle(MiniEngine::TravelContext& _context, 
		const MiniEngine::Vector3& _pos, 
		const MiniEngine::Vector3& _dir, 
		const float _dist,
		const float _hMultiplier = 2.0f, 
		const bool _bExcludeGroundActor = false);

	bool CheckLedge(
		MiniEngine::TravelContext& _context,
		const MiniEngine::Vector3& _pos,
		const MiniEngine::Vector3& _dir,
		const float _radius, 
		const float _dist,
		MiniEngine::Physics::RaycastResult& _outResult);

	// 장애물 높이 측정
	// 횟수는 config를 통해서 조절
	uint8_t MeasureObstacleHeight(
		MiniEngine::TravelContext& _context, 
		const MiniEngine::Vector3& _dir);

	// 꼭대기에서 전방으로 0.5 씩 나아가며 하향 레이로 딛을 면이 이어지는지 잰다.
	// 결과는 {0, 0.5, 1.0}
	void MeasureObstacleDepth(
		MiniEngine::TravelContext& _context,
		const MiniEngine::Vector3& _dir);

}