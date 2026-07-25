#pragma once
#include "Scene/Actor.h"

namespace MiniEngine
{
	class Input;
	class Controller;
	class Pawn : public Actor
	{
	public:
		virtual void OnPossessed(std::shared_ptr<Controller> _pController, Input& _input);
		
		std::shared_ptr<Controller> GetControllerActor() { return m_pController.lock(); }

	private:
		std::weak_ptr<Controller> m_pController;
	};
}