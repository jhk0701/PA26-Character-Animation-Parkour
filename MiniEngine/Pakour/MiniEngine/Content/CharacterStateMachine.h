#pragma once
#include <memory>
#include "Scene/Component.h"
#include "Content/Character.h"

class CharacterState
{
public:
	CharacterState() {};
	virtual ~CharacterState() {};

	void RegisterMachine(std::shared_ptr<CharacterStateMachine> _machine) { m_machine = _machine; }

	virtual void OnStart() = 0;
	virtual void OnEnd() = 0;

	// Transition이 빈번한 경우 OnStart, OnEnd를 계속 반복하지 않고, 대신 새로고침을 호출
	virtual void Refresh() {};

	virtual void Tick(float _dt) = 0;
	virtual void ProcessPerceptionResult(const Character::PerceptedObstacleInfo& _result) = 0;
	virtual void CheckState() {};

protected:
	std::shared_ptr<CharacterStateMachine> GetMachine() { return m_machine.lock(); }

	void DefaultMovement(float _dt);
	void DefaultCameraRotate(float _dt);
	void DefaultProcessPerceptionResult(const Character::PerceptedObstacleInfo& _info);

private:
	std::weak_ptr<CharacterStateMachine> m_machine;
};

// 캐릭터 상태에 따른 처리용도
class CharacterStateMachine : public MiniEngine::Component, public std::enable_shared_from_this<CharacterStateMachine>
{
public:
	virtual void Tick(float _dt) override;

	void RegisterStates(std::vector<std::shared_ptr<CharacterState>>&& _states);
	void Transition(uint8_t _nextID);
	void ProcessPerceptionResult(const Character::PerceptedObstacleInfo& _info);

	std::shared_ptr<Character> GetCharacter();

private:
	uint8_t m_curState{ 0 };
	std::vector<std::shared_ptr<CharacterState>> m_states;
};
