#pragma once

namespace MiniEngine 
{
	// 태그 구성 예시 : Obstacle,Wall,OverHead
	struct Tag
	{
	private:
		enum 
		{
			MAX_LAYER_CNT	= 8,	// 8개로 제한둘 것. 그 이상 계층이 깊어지는거면 구조를 다시 생각해봐야 함. 현재도 3~5까지 예상중
			TAG_DIVIDER		= ','	// ','으로 구분
		};
	public:
		Tag();
		// Tag(const std::string& _fullTag); // 문자열 비교보단 int 비교가 더 저렴하므로 변경

		bool Has(const uint8_t _tag) const;
		bool Match(const uint8_t _idx, const uint8_t _tag) const;

		Tag& operator+=(const uint8_t _tag) 
		{
			m_tags.push_back(_tag);
			return *this;
		}

	private:
		std::vector<uint8_t> m_tags;
	};
}