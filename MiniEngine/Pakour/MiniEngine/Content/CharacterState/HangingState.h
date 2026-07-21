#include "Content/CharacterStateMachine.h"
#include "Content/ContentConfig.h"

class HangingState : public CharacterState
{
public:
	void OnStart() override;
	void OnEnd() override;
	void Tick(float _dt) override;

private:
	void CameraRotate(float _dt);
	void ProcessMovement(float _dt);
	bool CheckEnableToMove(Content::Config::ETagAct _tag);
};