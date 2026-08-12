#include "pch.h"
#include "Content/Perception/Decorator/CharacterDecorator.h"

#include "Content/ContentConfig.h"
#include "Content/Character.h"

#include "Core/Log.h"

namespace
{
    std::shared_ptr<Character> ToChar(std::shared_ptr<Actor> _actor)
    {
        return std::dynamic_pointer_cast<Character>(_actor);
    }
}

bool CharacterStateDecorator::Evaluate(const TravelContext& _context) const
{
    std::shared_ptr<Character> pChar = ToChar(_context.m_owner);
    if (!pChar)
        return false;

    return Compare((uint8_t)pChar->GetState());
}

bool InputVerticalDecorator::Evaluate(const TravelContext& _context) const
{
    std::shared_ptr<Character> pChar = ToChar(_context.m_owner);
    if (!pChar)
        return false;

    return Compare(pChar->GetInputDir().y);
}

bool InputHorizontalDecorator::Evaluate(const TravelContext& _context) const
{
    std::shared_ptr<Character> pChar = ToChar(_context.m_owner);
    if (!pChar)
        return false;

    return Compare(pChar->GetInputDir().x);
}
