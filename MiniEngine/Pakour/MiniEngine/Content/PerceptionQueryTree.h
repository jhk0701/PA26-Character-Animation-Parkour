#pragma once

namespace MiniEngine 
{
	class QueryNodeBase;
	struct TravelContext;
}

class PerceptionQueryTree 
{
public:
	std::shared_ptr<MiniEngine::QueryNodeBase> ConstructTree();
};
