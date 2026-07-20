#pragma once

namespace MiniEngine 
{
	class QueryNodeBase;
	struct TravelContext;
}

// 콘텐츠 코드에서 사용할 것
// 쿼리 트리 팩토리용도
class PerceptionQueryTree 
{
public:
	std::shared_ptr<MiniEngine::QueryNodeBase> ConstructTree();
};

