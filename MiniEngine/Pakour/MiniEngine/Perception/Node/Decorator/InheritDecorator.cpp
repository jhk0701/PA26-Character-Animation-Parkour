#include "pch.h"
#include "InheritDecorator.h"

namespace 
{
	struct ComparerTypeName
	{
		const char* Name;
		ECompareType Type;
	};

	constexpr ComparerTypeName COMPARER_TYPE_NAMES[] =
	{
		{ "Greater",	ECompareType::Greater },
		{ "GEqual",		ECompareType::GEqual },
		{ "Equal",		ECompareType::Equal },
		{ "LEqual",		ECompareType::LEqual },
		{ "Lesser",		ECompareType::Lesser },
	};
}

bool TryParseComparerType(const std::string& _name, uint8_t& _outTag)
{
	for (const ComparerTypeName& ENTRY : COMPARER_TYPE_NAMES)
	{
		if (_name == ENTRY.Name)
		{
			_outTag = (uint8_t)ENTRY.Type;
			return true;
		}
	}

	return false;
}

const char* GetComparerTypeName(uint8_t _tag)
{
	for (const ComparerTypeName& ENTRY : COMPARER_TYPE_NAMES)
	{
		if ((uint8_t)ENTRY.Type == _tag)
			return ENTRY.Name;
	}

	return "<invalid>";
}
