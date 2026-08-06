#include "pch.h"
#include "SelectUsingInputDirNode.h"
#include "Content/Perception/PerceptionNodeUtil.h"
#include "Content/Character.h"

#include "Core/Log.h"

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
	else if (INPUT_DIR.y < 1e-4f && INPUT_DIR.x < 1e-4f) // 방향 입력이 없을 경우 0번으로 강제
		return 0;

	return 3;
}

uint8_t SelectUsingInputVerticalNode::InvokeCondition(TravelContext& _context)
{
	std::shared_ptr<Character> pChar = PerceptionNodeUtil::ToChar(_context.m_owner);
	const Vector2 INPUT_DIR = pChar->GetInputDir();

	uint8_t r = 0;
	if (INPUT_DIR.y > 0)
		r = 0;
	else if (INPUT_DIR.y < 0)
		r = 2;
	else
		r = 1;

	MG_LOG_INFO("[SelectUsingInputVerticalNode::InvokeCondition] result : {}", r);

	return r;
}
