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

// 현재 캐릭터가 파쿠르 중이라면
// 파구르 중인 장애물에 손이나 발을 짚는 IK 실행
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

// 예외적 적용 상황 발생
// 벽면의 경우 현 애니메이션이 목표 위치에 손을 닿기 어려워 - 거리가 좁혀지지 않아 그냥 뻗고만 있음
// 그렇다고 베지어로 위치를 옮기는 게 너무 어색해짐
// 임의로 캐릭터 바로 앞 정면에 손을 대도록 유도
class CharacterIKInvokerFixedPoint : public MiniEngine::AnimNotifyState 
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