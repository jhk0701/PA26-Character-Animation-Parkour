#include "UI/UIBase.h"

class Character;
namespace MiniEngine { class Animator; }

class UIDebugConsole : public MiniEngine::UIBase 
{
public:
	void Construct() override;

	void SetCharacter(std::weak_ptr<Character> _char) { m_pChar = _char; }

protected:
	void DrawUI() override;

private:
	std::weak_ptr<Character> m_pChar;
	void DrawGraph(std::shared_ptr<MiniEngine::Animator>& _pAnim);
};