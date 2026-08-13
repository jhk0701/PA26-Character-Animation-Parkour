#pragma once

#include "Asset/DataAsset.h"
#include "Core/Math.h"
#include "Animation/ActionClip.h"

// 노티파이 1개 전체 데이터
// 우선은 한번에 다 몰아서 사용
struct AnimNotifyData
{
	std::string NotifyClass;

	// AnimNotify는 TimeStart만, AnimNotifyState는 둘다 TimeStart, TimeEnd
	float TimeStart{ 0.0f };
	float TimeEnd{ 0.0f };

	bool bEnable{ true };			// EnableCollisionObstacle
	uint8_t TargetState{ 0 };		// TransitionState
	uint8_t Limb{ 0 };				// CharacterIKInvoker / CharacterIKInvokerFixedPoint
	std::vector<uint8_t> Limbs;		// CharacterIKEnabler

	float ProperDistance{ 1.0f };	// CorrectRootMotion
	uint8_t CorrectAxis{ 0 };		// ECorrectAxis

	float AlphaFrom{ 0.0f };		// CharacterIKEnabler
	float AlphaTo{ 1.0f };

	MiniEngine::Vector3 Vector{ 0.0f, 0.0f, 0.0f };		// 방향 + 포지션 표현용도
	MiniEngine::Vector3 Offset{ 0.0f, 0.0f, 0.0f };		// IK 노티파이
	MiniEngine::Vector3 MidOffset{ 0.0f, 0.0f, 0.0f };	// BezierCorrectRootMotion
	MiniEngine::Vector3 EndOffset{ 0.0f, 0.0f, 0.0f };	// BezierCorrectRootMotion
	MiniEngine::Vector3 RotateDeg{ 0.0f, 0.0f, 0.0f };	// CorrectFixedRotation
	std::vector<bool> AxisMask{ true, true, true }; // BezierCorrectRootMotion
};

struct BlendSampleData
{
	MiniEngine::Vector2 Coord{ 0.0f, 0.0f };
	int ClipIndex{ 0 };
};

struct LocomotionData
{
	uint8_t State{ 0 };		// Character::EState
	std::vector<BlendSampleData> Samples;
};

// 액션 기본 데이터 구성
struct ActionData
{
	std::vector<uint8_t> Tags;	// ETagAct
	int ClipIndex{ 0 };
	MiniEngine::ActionClip::RootMotionConfig RootMotion;

	float Speed{ 1.0f };			// 재생 속도
	float StartOffset{ 0.0f }; 
	float EndOffset{ 0.0f };	
	std::vector<AnimNotifyData> Notifies;
};

// Datas/CharacterActionClips.json 파싱 결과
class CharacterAnimData : public MiniEngine::DataAsset
{
public:
	void Load(const json& _data) override;

	bool IsValid() const { return m_bValid; }
	const std::vector<LocomotionData>& GetLocomotions() const { return m_locomotions; }
	const std::vector<ActionData>& GetActions() const { return m_actions; }

private:
	bool m_bValid{ false };
	std::vector<LocomotionData> m_locomotions;
	std::vector<ActionData> m_actions;
};
