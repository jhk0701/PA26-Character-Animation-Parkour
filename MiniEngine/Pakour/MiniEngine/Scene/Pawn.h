#pragma once
#include "Scene/Actor.h"

namespace MiniEngine
{
	class Input;
	class Controller;
	class Pawn : public Actor
	{
	public:
		virtual void OnPossessed(Input& _input) {};
	
	protected:
		std::shared_ptr<Controller> GetController() { return m_pController.lock(); }

	private:
		std::weak_ptr<Controller> m_pController;
	};
}