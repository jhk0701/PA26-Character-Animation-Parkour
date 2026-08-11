#include "pch.h"
#include "Content/Perception/TransitionCharacterFSMNode.h"
#include "Perception/Node/PerceptionNodeUtil.h"
#include "Content/Character.h"

EPerceptionResult TransitionCharacterFSMNode::InvokeTask(TravelContext& _context, TravelResult& _result)
{
	std::shared_ptr<Character> pChar = PerceptionNodeUtil::ToChar(_context.m_owner);
	pChar->TransitionStateMachine(m_state);

	return EPerceptionResult::Succeess;
}