#include "pch.h"
#include "Scene/Tag.h"

namespace MiniEngine 
{
	Tag::Tag()
	{
		m_tags.reserve(MAX_LAYER_CNT);
	}

	bool Tag::Has(const uint8_t _tag) const
	{
		for (const uint8_t t : m_tags)
		{
			if (t == _tag)
				return true;
		}

		return false;
	}

	bool Tag::GetTagAt(const uint8_t _idx, uint8_t& _outTag) const
	{
		if (m_tags.size() <= _idx)
			return false;
		
		_outTag = m_tags[_idx];
		return true;
	}

	bool Tag::Match(const uint8_t _idx, const uint8_t _tag) const
	{
		if (m_tags.size() <= _idx)
			return false;

		return m_tags[_idx] == _tag;
	}
}
