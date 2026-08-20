#pragma once

namespace MiniEngine 
{
	struct Tag
	{
	private:
		enum 
		{
			INIT_RESERVE_CNT	= 5,
		};
	public:
		Tag();

		void SetTag(const uint8_t _idx, const uint8_t _tag);
		bool Has(const uint8_t _tag) const;
		bool GetTagAt(const uint8_t _idx, uint8_t& _outTag) const;
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