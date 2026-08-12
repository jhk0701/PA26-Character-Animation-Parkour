#include "pch.h"
#include "Content/Perception/CheckObstacleNode.h"
#include "Scene/Actor.h"

#include "Perception/Interface/IPerceptionProcessor.h"
#include "Perception/PerceptionComponent.h"
#include "Perception/Node/PerceptionNodeUtil.h"

#include "Core/Log.h"

bool CheckObstacleNode::InvokeCondition(TravelContext& _context)
{
	const Transform& TF = _context.m_owner->GetRoot()->localTransform;
	std::shared_ptr<IPerceptionProcessor> pProcessor = std::dynamic_pointer_cast<IPerceptionProcessor>(_context.m_owner);
	assert(!pProcessor);

	Vector3 dir(0.0f);
	PerceptionNodeUtil::LocalizeDirection(TF, m_dir, dir);
	
	Vector3 pos(0.0f);
	PerceptionNodeUtil::LocalizePosition(TF, m_startOffset, pos);

	const float DIST = pProcessor->GetPerceptionConfig().maxObstacleDetectDist;
	
	return PerceptionNodeUtil::CheckObstacle(_context, pos, dir, DIST, m_heightMultipier, true);
}
