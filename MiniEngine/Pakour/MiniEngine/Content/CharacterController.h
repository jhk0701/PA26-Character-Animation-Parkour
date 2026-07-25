#pragma once
#include "Scene/Controller.h"

class Character;
class CharacterController : public MiniEngine::Controller
{
protected:
	void OnPossessed(MiniEngine::Input& _input) override;

private:
	std::shared_ptr<Character> GetChar();
};