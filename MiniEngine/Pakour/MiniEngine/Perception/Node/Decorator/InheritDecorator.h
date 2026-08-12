#pragma once
#include "Perception/PerceptionComponent.h"

using namespace MiniEngine;

enum class ECompareType : uint8_t
{
	Greater,
	GEqual,
	Equal,
	LEqual,
	Lesser
};

template<typename T>
class CompareWithValueDecorator : public PerceptionDecorator 
{
public:
	void SetValue(const T _val) { m_value = _val; }
	T GetValue() const { return m_value; }

	void SetComparer(ECompareType _comparer) { m_comparer = _comparer; }
	ECompareType GetComparer() const { return m_comparer; }

	bool Compare(const T& _t) const;

private:
	T m_value;
	ECompareType m_comparer{ ECompareType::Greater; }
};

template<typename T>
inline bool CompareWithValueDecorator<T>::Compare(const T& _t) const
{
	switch (m_comparer)
	{
	case ECompareType::Greater:
		return m_value < _t;
	case ECompareType::GEqual:
		return m_value <= _t;
	case ECompareType::Equal:
		return m_value == _t;
	case ECompareType::LEqual:
		return m_value >= _t;
	case ECompareType::Lesser:
		return m_value > _t;
	default:
		return false;
	}
}
