#include "Content/CharacterStateMachine.h"

class HangingState : public CharacterState
{
public:
	void OnStart() override;
	void OnEnd() override;
	void Tick(float _dt) override;
	void CheckState() override;

private:
	void CameraRotate(float _dt);
	void ProcessMovement(float _dt);
};