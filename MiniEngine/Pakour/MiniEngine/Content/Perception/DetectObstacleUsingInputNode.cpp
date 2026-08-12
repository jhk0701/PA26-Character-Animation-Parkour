#include "pch.h"
#include "Content/Perception/DetectObstacleUsingInputNode.h"
#include "Content/Character.h"

EPerceptionResult DetectObstacleUsingInputNode::InvokeTask(TravelContext& _context, TravelResult& _result)
{
	std::shared_ptr<Character> pChar = std::dynamic_pointer_cast<Character>(_context.m_owner);
	if (pChar == nullptr)
		return EPerceptionResult::Fail;

	const Vector2 INPUT = pChar->GetInputDir();
	Vector3 newDir(0.0f);

	if (std::fabs(INPUT.x) > 1e-4f) 
		newDir.x = INPUT.x > 0.0f ? 1.0f : -1.0f;

	if (std::fabs(INPUT.y) > 1e-4f)
		newDir.y = INPUT.y > 0.0f ? 1.0f : -1.0f;

	SetDirection(newDir);

	return DetectObstacleSphereNode::InvokeTask(_context, _result);
}
