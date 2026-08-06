#include "pch.h"
#include "Content/Processor/ProcessorConditionUtil.h"
#include "Content/ContentConfig.h"
#include "Content/Character.h"

using namespace MiniEngine;
using namespace Content::Config;

namespace ProcessorConditionUtil 
{
	std::shared_ptr<Character> ToChar(std::shared_ptr<MiniEngine::Actor> _actor)
	{
		return std::dynamic_pointer_cast<Character>(_actor);
	}

}

