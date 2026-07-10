#include "pch.h"
#include "Scene/Tag.h"

namespace MiniEngine 
{
	Tag::Tag()
	{
		m_tags.reserve(MAX_LAYER_CNT);
	}

	//Tag::Tag(const std::string& _fullTag) : Tag()
	//{
	//	long long offset = 0;
	//	long long found = 0;
	//	do
	//	{
	//		found = _fullTag.find(TAG_DIVIDER, offset);
	//		if (found == std::string::npos)
	//		{
	//			// 발견된 태그 없음 -> 마지막 태그 처리
	//			m_tags.push_back(_fullTag.substr(offset, _fullTag.length() - offset));
	//			break;
	//		}

	//		m_tags.push_back(_fullTag.substr(offset, found - offset));
	//		offset = found + 1;
	//	} 
	//	while (offset < _fullTag.length());
	//}

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
