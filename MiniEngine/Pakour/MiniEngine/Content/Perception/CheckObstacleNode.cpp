#include "pch.h"
#include "Content/Perception/CheckObstacleNode.h"
#include "Content/Perception/PerceptionNodeUtil.h"
#include "Content/Character.h"
#include "Content/Data/CharacterConfigData.h"
#include "Core/Log.h"

bool CheckObstacleNode::InvokeCondition(TravelContext& _context)
{
	const Transform& TF = _context.m_owner->GetRoot()->localTransform;
	std::shared_ptr<Character> pChar = PerceptionNodeUtil::ToChar(_context.m_owner);

	Vector3 offset(0.0f);
	offset += m_startOffset.x * TF.Right();
	offset += m_startOffset.y * TF.Up();
	offset += m_startOffset.z * TF.Forward();

	const Vector3 POS = PerceptionNodeUtil::GetCharacterCenterPosition(_context) + offset;
	const float DIST = pChar->GetPerceptionConfig().maxObstacleDetectDist;
	
	// 정면으로 먼저 확인
	return PerceptionNodeUtil::CheckObstacle(_context, POS, TF.Forward(), DIST, m_heightMultipier, true);
}
