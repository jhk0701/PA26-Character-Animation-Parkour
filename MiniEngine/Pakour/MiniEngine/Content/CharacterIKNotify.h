#pragma once
#include "Animation/AnimNotify.h"

class Character;
class CharacterIKEnabler : public MiniEngine::AnimNotifyState
{
public:
	void OnStart(MiniEngine::AnimNotifyParam& _param) override;
	void Activate(float _dt, MiniEngine::AnimNotifyParam& _param) override;
	void OnEnd(MiniEngine::AnimNotifyParam& _param) override;

	void SetIKType(uint8_t _type) { m_ikType = _type; }
	void SetFromTo(float _from, float _to) { m_from = _from; m_to = _to; }

private:
	uint8_t m_ikType;
	Character* m_pChar{ nullptr };

	float m_from{ 0.0f };
	float m_to{ 1.0f };
};

class CharacterIKInvoker : public MiniEngine::AnimNotifyState
{
public:
	void OnStart(MiniEngine::AnimNotifyParam& _param) override;
	void Activate(float _dt, MiniEngine::AnimNotifyParam& _param) override;

	void SetIKType(uint8_t _type) { m_ikType = _type; }

private:
	uint8_t m_ikType;
	Character* m_pChar{ nullptr };
};