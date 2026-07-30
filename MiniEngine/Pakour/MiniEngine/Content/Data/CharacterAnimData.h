#pragma once

#include "Asset/DataAsset.h"
#include "Core/Math.h"
#include "Animation/ActionClip.h"

// 노티파이 1개의 저작값.
// 클래스마다 쓰는 필드만 유효하다(총 파라미터가 13개뿐이라 클래스별 파생 대신 합집합으로 둔다).
struct AnimNotifyData
{
	std::string NotifyClass;

	// AnimNotify 는 TimeStart 만, AnimNotifyState 는 [TimeStart, TimeEnd] 를 쓴다
	float TimeStart{ 0.0f };
	float TimeEnd{ 0.0f };

	bool bEnable{ true };			// EnableCollisionObstacle
	uint8_t TargetState{ 0 };		// TransitionState
	uint8_t Limb{ 0 };				// CharacterIKInvoker / CharacterIKInvokerFixedPoint
	std::vector<uint8_t> Limbs;		// CharacterIKEnabler

	float ProperDistance{ 1.0f };	// CorrectRootMotion
	float LerpWeight{ 0.5f };
	float DeltaIntensity{ 1.0f };
	uint8_t CorrectAxis{ 0 };		// ECorrectAxis

	float BezierY{ 0.0f };			// BezierCorrectRootMotion

	float AlphaFrom{ 0.0f };		// CharacterIKEnabler
	float AlphaTo{ 1.0f };

	MiniEngine::Vector3 Offset{ 0.0f, 0.0f, 0.0f };		// IK 노티파이
	MiniEngine::Vector3 EndOffset{ 0.0f, 0.0f, 0.0f };	// BezierCorrectRootMotion
	MiniEngine::Vector3 RotateDeg{ 0.0f, 0.0f, 0.0f };	// CorrectFixedRotation
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

struct ActionData
{
	std::vector<uint8_t> Tags;	// ETagAct — 여러 태그가 같은 ActionClip 을 공유한다
	int ClipIndex{ 0 };
	MiniEngine::ActionClip::RootMotionConfig RootMotion;
	std::vector<AnimNotifyData> Notifies;
};

// Datas/CharacterActionClips.json 파싱 결과.
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
