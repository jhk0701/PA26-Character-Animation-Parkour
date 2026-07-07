#include "pch.h"
#include "Scene/Tag.h"

namespace MiniEngine 
{
	Tag::Tag(const std::string& _fullTag)
	{
		m_Tags.reserve(MAX_LAYER_CNT);

		long long offset = 0;
		long long found = 0;
		do
		{
			found = _fullTag.find(TAG_DIVIDER, offset);
			if (found == std::string::npos)
			{
				// 발견된 태그 없음 -> 마지막 태그 처리
				m_Tags.push_back(_fullTag.substr(offset, _fullTag.length() - offset));
				break;
			}

			m_Tags.push_back(_fullTag.substr(offset, found - offset));
			offset = found + 1;
		} 
		while (offset < _fullTag.length());
	}

	bool Tag::HasTag(const std::string& _tag) const
	{
		for (const std::string& s : m_Tags)
		{
			if (s == _tag)
				return true;
		}

		return false;
	}

}
