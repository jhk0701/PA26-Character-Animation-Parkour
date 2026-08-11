#pragma once
#include "Perception/ProcessorComponent.h"

namespace MiniEngine { class Actor; }
class Character;

namespace ProcessorConditionUtil 
{
	std::shared_ptr<Character> ToChar(std::shared_ptr<MiniEngine::Actor> _actor);
}

template<typename T>
class CompareWithValueCondition : public MiniEngine::ProcessCondition 
{
public:
	void SetValue(T _val) { m_value = _val; }
	T GetValue() const { return m_value; }

private:
	T m_value;
};