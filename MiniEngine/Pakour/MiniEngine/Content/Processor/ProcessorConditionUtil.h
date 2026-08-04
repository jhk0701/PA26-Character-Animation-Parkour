#pragma once

namespace MiniEngine { class Actor; }
class Character;

namespace ProcessorConditionUtil 
{
	std::shared_ptr<Character> ToChar(std::shared_ptr<MiniEngine::Actor> _actor);

}
