#pragma once
#include "UI/UIBase.h"

using namespace MiniEngine;

class Character;
class UIDebugPerceptionResult : public UIBase
{
public:
	void Construct() override;
	void SetCharacter(std::weak_ptr<Character> _char) { m_pChar = _char; }

protected:
	void DrawUI() override;
	
private:
	std::weak_ptr<Character> m_pChar;
};