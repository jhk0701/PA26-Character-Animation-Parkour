#pragma once
#include "Scene/Actor.h"

namespace MiniEngine
{
	class Input;
	class Pawn;
	class Controller : public Actor
	{
	public:
		void Construct();
		void Possess(std::shared_ptr<Pawn> _pawn);
		void Unpossess();

		bool IsPossessing() const { return m_possesed.expired() == false; }

	protected:
		virtual void OnPossessed(Input& _input);
		std::shared_ptr<Pawn> GetPossessed() { return m_possesed.lock(); }

	private:
		std::weak_ptr<Pawn> m_possesed;
	};
}