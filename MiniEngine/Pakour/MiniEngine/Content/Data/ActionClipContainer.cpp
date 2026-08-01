#include "pch.h"
#include "Content/Data/ActionClipContainer.h"
#include "Content/Data/CharacterAnimData.h"

#include "Core/Log.h"
#include "Asset/SkinnedMesh.h"
#include "Animation/IK/LimbIKComponent.h"

#include "Animation/Animator.h"
#include "Animation/BlendClip.h"
#include "Animation/ActionClip.h"
#include "Animation/AnimNotify.h"

#include "Content/ContentConfig.h"
#include "Content/Character.h"

// 노티파이
#include "Content/JumpTiming.h"
#include "Content/EnableCollisionObstacle.h"
#include "Content/TransitionState.h"
#include "Content/CorrectRootMotion.h"
#include "Content/CorrectRotation.h"
#include "Content/CharacterIKNotify.h"

#include <functional>

using namespace MiniEngine;
using namespace Content::Config;

namespace
{
#pragma region Notify Registry

	using NotifyCreator = std::function<std::shared_ptr<IAnimNotify>(const AnimNotifyData&)>;

	// 새 노티파이 클래스를 추가하면 여기에 등록해야 json 에서 쓸 수 있다.
	const std::unordered_map<std::string, NotifyCreator>& NotifyRegistry()
	{
		static const std::unordered_map<std::string, NotifyCreator> REGISTRY =
		{
			{ "JumpTiming",
				[](const AnimNotifyData& _data)
				{
					std::shared_ptr<JumpTiming> pNotify = std::make_shared<JumpTiming>();
					pNotify->SetTime(_data.TimeStart);
					return pNotify;
				} },

			{ "EnableCollisionObstacle",
				[](const AnimNotifyData& _data)
				{
					std::shared_ptr<EnableCollisionObstacle> pNotify = std::make_shared<EnableCollisionObstacle>();
					pNotify->SetTime(_data.TimeStart);
					pNotify->SetEnable(_data.bEnable);
					return pNotify;
				} },

			{ "TransitionState",
				[](const AnimNotifyData& _data)
				{
					std::shared_ptr<TransitionState> pNotify = std::make_shared<TransitionState>();
					pNotify->SetTime(_data.TimeStart);
					pNotify->SetState(_data.TargetState);
					return pNotify;
				} },

			{ "CorrectRootMotion",
				[](const AnimNotifyData& _data)
				{
					std::shared_ptr<CorrectRootMotion> pNotify = std::make_shared<CorrectRootMotion>();
					pNotify->SetTime(_data.TimeStart, _data.TimeEnd);
					pNotify->SetProperDistance(_data.ProperDistance);
					pNotify->SetLerpWeight(_data.LerpWeight);
					// pNotify->SetDeltaIntensity(_data.DeltaIntensity);
					pNotify->SetCorrectAxis((ECorrectAxis)_data.CorrectAxis);
					return pNotify;
				} },

			{ "BezierCorrectRootMotion",
				[](const AnimNotifyData& _data)
				{
					std::shared_ptr<BezierCorrectRootMotion> pNotify = std::make_shared<BezierCorrectRootMotion>();
					pNotify->SetTime(_data.TimeStart, _data.TimeEnd);
					pNotify->SetMidOffset(_data.MidOffset); // pNotify->SetBezierY(_data.BezierY);
					pNotify->SetEndOffset(_data.EndOffset);
					return pNotify;
				} },

			{ "CorrectFixedRotation",
				[](const AnimNotifyData& _data)
				{
					std::shared_ptr<CorrectFixedRotation> pNotify = std::make_shared<CorrectFixedRotation>();
					pNotify->SetTime(_data.TimeStart, _data.TimeEnd);
					pNotify->SetRotateDeg(_data.RotateDeg);
					return pNotify;
				} },

			{ "CorrectRotationTowardObstacle",
				[](const AnimNotifyData& _data)
				{
					std::shared_ptr<CorrectRotationTowardObstacle> pNotify = std::make_shared<CorrectRotationTowardObstacle>();
					pNotify->SetTime(_data.TimeStart, _data.TimeEnd);
					return pNotify;
				} },

			{ "CharacterIKEnabler",
				[](const AnimNotifyData& _data)
				{
					std::shared_ptr<CharacterIKEnabler> pNotify = std::make_shared<CharacterIKEnabler>();
					pNotify->SetTime(_data.TimeStart, _data.TimeEnd);
					pNotify->SetFromTo(_data.AlphaFrom, _data.AlphaTo);
					pNotify->SetIKType(std::vector<uint8_t>(_data.Limbs));
					return pNotify;
				} },

			{ "CharacterIKInvoker",
				[](const AnimNotifyData& _data)
				{
					std::shared_ptr<CharacterIKInvoker> pNotify = std::make_shared<CharacterIKInvoker>();
					pNotify->SetTime(_data.TimeStart, _data.TimeEnd);
					pNotify->SetIKType(_data.Limb);
					pNotify->SetPositionOffset(_data.Offset);
					return pNotify;
				} },

			{ "CharacterIKInvokerFixedPoint",
				[](const AnimNotifyData& _data)
				{
					std::shared_ptr<CharacterIKInvokerFixedPoint> pNotify = std::make_shared<CharacterIKInvokerFixedPoint>();
					pNotify->SetTime(_data.TimeStart, _data.TimeEnd);
					pNotify->SetIKType(_data.Limb);
					pNotify->SetPositionOffset(_data.Offset);
					return pNotify;
				} },
		};

		return REGISTRY;
	}

#pragma endregion

	bool IsValidClipIndex(const SkinnedMesh& _mesh, int _index)
	{
		return _index >= 0 && _index < (int)_mesh.GetClips().size();
	}
}

void ActionClipContainer::LoadActionClips(ActionClipLoadParam& _param)
{
	if (_param.pAnimData == nullptr || _param.pAnimData->IsValid() == false)
	{
		MG_LOG_ERROR("[ActionClipContainer] anim data is invalid. no clip loaded.");
		return;
	}

	std::shared_ptr<Animator> pAnim = _param.pAnim;
	std::shared_ptr<SkinnedMesh> skinnedMesh = _param.pSources;
	std::unordered_map<uint8_t, std::shared_ptr<ActionClip>>& mapActions = *_param.pMaps;

	const CharacterAnimData& DATA = *_param.pAnimData;
	const std::unordered_map<std::string, NotifyCreator>& REGISTRY = NotifyRegistry();

	// 로코모션
	// BaseTrack EState 순서에 맞춤
	{
		std::vector<const LocomotionData*> ordered;
		ordered.reserve(DATA.GetLocomotions().size());

		for (const LocomotionData& LOCO : DATA.GetLocomotions())
			ordered.push_back(&LOCO);

		std::sort(ordered.begin(), ordered.end(),
			[](const LocomotionData* _a, const LocomotionData* _b) { return _a->State < _b->State; });

		uint8_t expectedState = 0;
		for (const LocomotionData* pLoco : ordered)
		{
			if (pLoco->State != expectedState)
			{
				MG_LOG_ERROR("[ActionClipContainer] locomotion state '{}' is missing. base track will be misaligned.",
					Character::GetStateName(expectedState));
				return;
			}

			std::shared_ptr<BlendClip> pBlend = std::make_shared<BlendClip>((int)pLoco->Samples.size());

			for (const BlendSampleData& SAMPLE : pLoco->Samples)
			{
				if (IsValidClipIndex(*skinnedMesh, SAMPLE.ClipIndex) == false)
				{
					MG_LOG_ERROR("[ActionClipContainer] locomotion '{}' has out of range clip index {}.",
						Character::GetStateName(pLoco->State), SAMPLE.ClipIndex);
					return;
				}

				pBlend->AddAnimClip(SAMPLE.Coord, skinnedMesh->GetClipPtr(SAMPLE.ClipIndex));
			}

			pAnim->AddBaseLocomotion(pBlend);
			++expectedState;
		}
	}

	// ActionClip 구성
	for (const ActionData& ACTION : DATA.GetActions())
	{
		const char* const TAG_NAME = GetTagActName(ACTION.Tags[0]);

		if (IsValidClipIndex(*skinnedMesh, ACTION.ClipIndex) == false)
		{
			MG_LOG_ERROR("[ActionClipContainer] action '{}' has out of range clip index {}.",
				TAG_NAME, ACTION.ClipIndex);
			return;
		}

		std::shared_ptr<ActionClip> pActionClip = std::make_shared<ActionClip>();
		pActionClip->AddClip(skinnedMesh->GetClipPtr(ACTION.ClipIndex));
		pActionClip->SetApplyRootBone(ACTION.RootMotion);

		for (const AnimNotifyData& NOTIFY : ACTION.Notifies)
		{
			auto itCreator = REGISTRY.find(NOTIFY.NotifyClass);
			if (itCreator == REGISTRY.end())
			{
				MG_LOG_ERROR("[ActionClipContainer] action '{}' has unknown notify class '{}'.",
					TAG_NAME, NOTIFY.NotifyClass);
				return;
			}

			pActionClip->AddNotify(itCreator->second(NOTIFY));
		}

		for (uint8_t tag : ACTION.Tags)
			mapActions[tag] = pActionClip;
	}

	MG_LOG_INFO("[ActionClipContainer] loaded {} locomotions, {} actions.",
		DATA.GetLocomotions().size(), DATA.GetActions().size());
}
