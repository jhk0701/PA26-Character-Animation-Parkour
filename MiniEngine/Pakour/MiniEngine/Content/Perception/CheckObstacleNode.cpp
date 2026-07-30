#include "pch.h"
#include "Content/Perception/CheckObstacleNode.h"
#include "Content/Perception/PerceptionNodeUtil.h"
#include "Content/Character.h"

bool CheckObstacleNode::InvokeCondition(TravelContext& _context)
{
	// 정면으로 먼저 확인
	std::shared_ptr<Character> pChar = PerceptionNodeUtil::ToChar(_context.m_owner);
	const Transform& TF = pChar->GetRoot()->localTransform;

	Vector3 offset(0.0f);
	offset += m_startOffset.x * TF.Right();
	offset += m_startOffset.y * TF.Up();
	offset += m_startOffset.z * TF.Forward();

	const Vector3 POS = PerceptionNodeUtil::GetCharacterCenterPosition(_context) + offset;
	const float DIST = pChar->GetPerceptionConfig().maxObstacleDetectDist;

	return PerceptionNodeUtil::CheckObstacle(_context, POS, TF.Forward(), DIST, m_heightMultipier, true);
}
