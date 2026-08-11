#include "pch.h"
#include "Content/Perception/SelectCharacterStateNode.h"
#include "Content/Character.h"
#include "Perception/Node/PerceptionNodeUtil.h"

uint8_t SelectCharacterStateNode::InvokeCondition(TravelContext& _context)
{
	std::shared_ptr<Character> pChar = PerceptionNodeUtil::ToChar(_context.m_owner);
	if (pChar == nullptr)
		return (uint8_t)0;

	return (uint8_t)pChar->GetState();
}
