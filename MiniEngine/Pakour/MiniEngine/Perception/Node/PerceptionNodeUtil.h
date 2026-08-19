#pragma once
#include "Perception/PerceptionComponent.h"

namespace MiniEngine 
{ 
	class Actor; 
	class IObstacle; 
	struct Transform; 
}

namespace PerceptionNodeUtil 
{
	MiniEngine::IObstacle* ToIObstacle(void* _p);

	template<typename THit>
	void FillFromResult(MiniEngine::TravelContext& _context, const THit& _result)
	{
		_context.intermediate.pObstacle = ToIObstacle(_result.GetActor());
		_context.intermediate.obstacleHitPos = _result.m_pos;
		_context.intermediate.obstacleHitNrm = _result.m_nrm;
		_context.intermediate.obstacleDistance = _result.m_distance;
		_context.intermediate.obstacleLedge = _result.m_pos.y;
	}

	void LocalizePosition(const MiniEngine::Transform& _inTf, const MiniEngine::Vector3& _inOffset, MiniEngine::Vector3& _outResult);
	void LocalizeDirection(const MiniEngine::Transform& _inTf, const MiniEngine::Vector3& _inDir, MiniEngine::Vector3& _outResult);
}