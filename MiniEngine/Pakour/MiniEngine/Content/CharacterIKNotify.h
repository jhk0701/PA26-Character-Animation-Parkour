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

// 현재 캐릭터가 파쿠르 중이라면
// 장애물을 인식한 지점은 vector3 포인트이므로, 벽면 등에서는 특정한 방향을 향하도록
class CharacterIKInvokerFixedDir : public MiniEngine::AnimNotifyState 
{
public:
	
	void OnStart(MiniEngine::AnimNotifyParam& _param) override;
	void Activate(float _dt, MiniEngine::AnimNotifyParam& _param) override;

	void SetIKType(uint8_t _type) { m_ikType = _type; }
	void SetPositionOffset(const MiniEngine::Vector3& _offset) { m_posOffset = _offset; }
	void SetDir(const MiniEngine::Vector3& _dir) { m_dir = _dir; }

private:
	uint8_t m_ikType;
	Character* m_pChar{ nullptr };
	MiniEngine::Vector3 m_posOffset;
	MiniEngine::Vector3 m_dir;
};