#include "Content/CharacterStateMachine.h"

class LandingState : public CharacterState
{
public:
	void OnStart() override;
	void OnEnd() override;
	void Tick(float _dt) override;
	void CheckState() override;
};
