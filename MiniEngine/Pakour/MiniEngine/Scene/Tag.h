#pragma once

namespace MiniEngine 
{
	// 레이어 구성 예시 : Obstacle,Wall,OverHead
	struct Tag
	{
	private:
		enum 
		{
			MAX_LAYER_CNT	= 8,	// 8개로 제한둘 것. 그 이상 계층이 깊어지는거면 구조를 다시 생각해봐야 함. 현재도 3~5까지 예상중
			TAG_DIVIDER		= ','	// ','으로 구분
		};
	public:
		std::vector<std::string> m_Tags;
		
		Tag(const std::string& _fullTag);
		bool HasTag(const std::string& _tag) const;
	};
}