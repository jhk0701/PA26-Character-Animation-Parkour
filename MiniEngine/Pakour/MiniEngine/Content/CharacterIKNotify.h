#pragma once
#include "Animation/AnimNotify.h"

class Character;
class CharacterIKEnabler : public MiniEngine::AnimNotifyState
{
public:
	void OnStart(MiniEngine::AnimNotifyParam& _param) override;
	void Activate(float _dt, MiniEngine::AnimNotifyParam& _param) override;
	void OnEnd(MiniEngine::AnimNotifyParam& _param) override;

	void SetIKType(std::vector<uint8_t>&& _ikTypes) { m_ikTypes = _ikTypes; }
	void SetFromTo(float _from, float _to) { m_from = _from; m_to = _to; }

private:
	std::vector<uint8_t> m_ikTypes;
	Character* m_pChar{ nullptr };

	float m_elapsedTime{ 0.0f };
	float m_from{ 0.0f };
	float m_to{ 1.0f };
};

class CharacterIKInvoker : public MiniEngine::AnimNotifyState
{
public:
	void OnStart(MiniEngine::AnimNotifyParam& _param) override;
	void Activate(float _dt, MiniEngine::AnimNotifyParam& _param) override;

	void SetIKType(uint8_t _type) { m_ikType = _type; }
	void SetPositionOffset(const MiniEngine::Vector3& _offset) { m_posOffset = _offset; }

private:
	uint8_t m_ikType;
	Character* m_pChar{ nullptr };
	MiniEngine::Vector3 m_posOffset;
};