#include "pch.h"
#include "SelectUsingInputDirNode.h"
#include "Content/Perception/PerceptionNodeUtil.h"
#include "Content/Character.h"

uint8_t SelectUsingInputDirNode::InvokeCondition(TravelContext& _context)
{
	std::shared_ptr<Character> pChar = PerceptionNodeUtil::ToChar(_context.m_owner);
	const Vector2 INPUT_DIR = pChar->GetInputDir();

	if (INPUT_DIR.y > 0)
		return 0;
	else if (INPUT_DIR.y < 0)
		return 1;
	else if (std::fabs(INPUT_DIR.x) > 1e-4f)
		return 2;

	return 3;
}
