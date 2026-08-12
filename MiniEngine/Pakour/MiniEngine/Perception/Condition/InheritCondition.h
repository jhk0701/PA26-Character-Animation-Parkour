#pragma once
#include "Perception/ProcessorComponent.h"

template<typename T>
class CompareWithValueCondition : public MiniEngine::ProcessCondition
{
public:
	void SetValue(T _val) { m_value = _val; }
	T GetValue() const { return m_value; }

private:
	T m_value;
};