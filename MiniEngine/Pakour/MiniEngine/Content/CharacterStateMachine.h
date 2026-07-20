#pragma once
#include <memory>
#include "Scene/Component.h"

class Character;
class CharacterState;

// 캐릭터 상태에 따른 처리용도
class CharacterStateMachine : public MiniEngine::Component, public std::enable_shared_from_this<CharacterStateMachine>
{
public:
	virtual void Tick(float _dt) override;

	void RegisterStates(std::vector<std::shared_ptr<CharacterState>>&& _states);
	void Transition(uint8_t _nextID);

	std::shared_ptr<Character> GetCharacter();

private:
	uint8_t m_curState{ 0 };
	std::vector<std::shared_ptr<CharacterState>> m_states;
};

class CharacterState
{
public:
	CharacterState();
	virtual ~CharacterState() {};

	void RegisterMachine(std::shared_ptr<CharacterStateMachine> _machine) { m_machine = _machine; }

	virtual void OnStart() = 0;
	virtual void Tick(float _dt) = 0;
	virtual void OnEnd() = 0;

protected:
	std::shared_ptr<CharacterStateMachine> GetMachine() { return m_machine.lock(); }

private:
	std::weak_ptr<CharacterStateMachine> m_machine;

};

class LandingState : public CharacterState 
{
public:
	void OnStart() override;
	void Tick(float _dt) override;
	void OnEnd() override;

private:
	void InputMovement(float _dt);
	void InputCamRotate(float _dt);
};

class HangingState : public CharacterState
{
public:
	void OnStart() override;
	void Tick(float _dt) override;
	void OnEnd() override;
};

class InAirState : public CharacterState
{
public:
	void OnStart() override;
	void Tick(float _dt) override;
	void OnEnd() override;
};