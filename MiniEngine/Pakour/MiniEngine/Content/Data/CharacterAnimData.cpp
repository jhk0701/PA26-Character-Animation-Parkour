#include "pch.h"
#include "Content/Data/CharacterAnimData.h"
#include "Core/Log.h"

#include "Animation/IK/LimbIKComponent.h"
#include "Content/ContentConfig.h"
#include "Content/Character.h"
#include "Content/CorrectRootMotion.h"

using namespace MiniEngine;
using namespace Content::Config;

namespace
{
	constexpr const char* LIMB_NAMES[] =
	{
		"LeftArm",
		"RightArm",
		"LeftLeg",
		"RightLeg",
	};

	static_assert(
		std::size(LIMB_NAMES) == (size_t)ELimbType::End,
		"LIMB_NAMES 가 ELimbType 과 어긋났다.");

	constexpr const char* CORRECT_AXIS_NAMES[] =
	{
		"XZ",
		"XY",
		"YZ",
		"None",
	};

	static_assert(
		std::size(CORRECT_AXIS_NAMES) == (size_t)ECorrectAxis::None + 1,
		"CORRECT_AXIS_NAMES 가 ECorrectAxis 와 어긋났다.");

	bool TryParseName(const char* const* _table, size_t _count, const std::string& _name, uint8_t& _out)
	{
		for (size_t i = 0; i < _count; ++i)
		{
			if (_name == _table[i])
			{
				_out = (uint8_t)i;
				return true;
			}
		}

		return false;
	}

	bool TryParseLimb(const std::string& _name, uint8_t& _out)
	{
		return TryParseName(LIMB_NAMES, std::size(LIMB_NAMES), _name, _out);
	}

	void ReadVec3(const json& _data, const char* _key, Vector3& _out)
	{
		auto it = _data.find(_key);
		if (it == _data.end() || it->is_array() == false || it->size() != 3)
			return;

		_out = Vector3((*it)[0].get<float>(), (*it)[1].get<float>(), (*it)[2].get<float>());
	}

	void ReadVec2(const json& _data, const char* _key, Vector2& _out)
	{
		auto it = _data.find(_key);
		if (it == _data.end() || it->is_array() == false || it->size() != 2)
			return;

		_out = Vector2((*it)[0].get<float>(), (*it)[1].get<float>());
	}

	// "time" 은 단발 노티파이면 스칼라, 구간 노티파이면 [start, end] 배열이다.
	void ReadTime(const json& _data, float& _outStart, float& _outEnd)
	{
		auto it = _data.find("time");
		if (it == _data.end())
			return;

		if (it->is_number())
		{
			_outStart = it->get<float>();
			_outEnd = _outStart;
			return;
		}

		if (it->is_array() && it->size() == 2)
		{
			_outStart = (*it)[0].get<float>();
			_outEnd = (*it)[1].get<float>();
		}
	}

	bool LoadNotify(const json& _data, AnimNotifyData& _out)
	{
		_out.NotifyClass = _data.value("class", std::string());
		if (_out.NotifyClass.empty())
		{
			MG_LOG_ERROR("[CharacterAnimData] notify needs 'class'.");
			return false;
		}

		ReadTime(_data, _out.TimeStart, _out.TimeEnd);

		_out.bEnable = _data.value("enable", _out.bEnable);
		_out.ProperDistance = _data.value("properDistance", _out.ProperDistance);
		_out.LerpWeight = _data.value("lerpWeight", _out.LerpWeight);
		// _out.DeltaIntensity = _data.value("deltaIntensity", _out.DeltaIntensity);
		// _out.BezierY = _data.value("bezierY", _out.BezierY);
		ReadVec3(_data, "offset", _out.Offset);
		ReadVec3(_data, "vector", _out.Vector);
		ReadVec3(_data, "midOffset", _out.MidOffset);
		ReadVec3(_data, "endOffset", _out.EndOffset);
		ReadVec3(_data, "rotateDeg", _out.RotateDeg);

		// fromTo : CharacterIKEnabler 의 알파 [from, to]
		auto itFromTo = _data.find("fromTo");
		if (itFromTo != _data.end() && itFromTo->is_array() && itFromTo->size() == 2)
		{
			_out.AlphaFrom = (*itFromTo)[0].get<float>();
			_out.AlphaTo = (*itFromTo)[1].get<float>();
		}

		const std::string STATE_NAME = _data.value("state", std::string());
		if (STATE_NAME.empty() == false
			&& Character::TryParseState(STATE_NAME, _out.TargetState) == false)
		{
			MG_LOG_ERROR("[CharacterAnimData] notify '{}' has unknown state '{}'.",
				_out.NotifyClass, STATE_NAME);
			return false;
		}

		const std::string LIMB_NAME = _data.value("limb", std::string());
		if (LIMB_NAME.empty() == false && TryParseLimb(LIMB_NAME, _out.Limb) == false)
		{
			MG_LOG_ERROR("[CharacterAnimData] notify '{}' has unknown limb '{}'.",
				_out.NotifyClass, LIMB_NAME);
			return false;
		}

		auto itLimbs = _data.find("limbs");
		if (itLimbs != _data.end() && itLimbs->is_array())
		{
			for (const json& element : *itLimbs)
			{
				uint8_t limb = 0;
				if (TryParseLimb(element.get<std::string>(), limb) == false)
				{
					MG_LOG_ERROR("[CharacterAnimData] notify '{}' has unknown limb '{}'.",
						_out.NotifyClass, element.get<std::string>());
					return false;
				}

				_out.Limbs.push_back(limb);
			}
		}

		const std::string AXIS_NAME = _data.value("correctAxis", std::string());
		if (AXIS_NAME.empty() == false
			&& TryParseName(CORRECT_AXIS_NAMES, std::size(CORRECT_AXIS_NAMES), AXIS_NAME, _out.CorrectAxis) == false)
		{
			MG_LOG_ERROR("[CharacterAnimData] notify '{}' has unknown correctAxis '{}'.",
				_out.NotifyClass, AXIS_NAME);
			return false;
		}

		return true;
	}
}

void CharacterAnimData::Load(const json& _data)
{
	m_bValid = false;
	m_locomotions.clear();
	m_actions.clear();

	try
	{
		// 로코모션
		auto itLoco = _data.find("locomotion");
		if (itLoco == _data.end() || itLoco->is_array() == false)
		{
			MG_LOG_ERROR("[CharacterAnimData] 'locomotion' array is missing.");
			return;
		}

		for (const json& element : *itLoco)
		{
			LocomotionData loco;

			const std::string STATE_NAME = element.value("state", std::string());
			if (Character::TryParseState(STATE_NAME, loco.State) == false)
			{
				MG_LOG_ERROR("[CharacterAnimData] locomotion has unknown state '{}'.", STATE_NAME);
				return;
			}

			auto itSamples = element.find("samples");
			if (itSamples == element.end() || itSamples->is_array() == false || itSamples->empty())
			{
				MG_LOG_ERROR("[CharacterAnimData] locomotion '{}' has no samples.", STATE_NAME);
				return;
			}

			loco.Samples.reserve(itSamples->size());
			for (const json& sampleElement : *itSamples)
			{
				BlendSampleData sample;
				ReadVec2(sampleElement, "coord", sample.Coord);
				sample.ClipIndex = sampleElement.value("clip", -1);

				if (sample.ClipIndex < 0)
				{
					MG_LOG_ERROR("[CharacterAnimData] locomotion '{}' sample needs 'clip'.", STATE_NAME);
					return;
				}

				loco.Samples.push_back(sample);
			}

			m_locomotions.push_back(std::move(loco));
		}

		// 액션
		auto itActions = _data.find("actions");
		if (itActions == _data.end() || itActions->is_array() == false)
		{
			MG_LOG_ERROR("[CharacterAnimData] 'actions' array is missing.");
			return;
		}

		m_actions.reserve(itActions->size());

		for (const json& element : *itActions)
		{
			ActionData action;

			auto itTags = element.find("tags");
			if (itTags == element.end() || itTags->is_array() == false || itTags->empty())
			{
				MG_LOG_ERROR("[CharacterAnimData] action needs 'tags'.");
				return;
			}

			for (const json& tagElement : *itTags)
			{
				uint8_t tag = 0;
				const std::string TAG_NAME = tagElement.get<std::string>();

				if (TryParseTagAct(TAG_NAME, tag) == false)
				{
					MG_LOG_ERROR("[CharacterAnimData] unknown action tag '{}'.", TAG_NAME);
					return;
				}

				action.Tags.push_back(tag);
			}

			action.ClipIndex = element.value("clip", -1);
			if (action.ClipIndex < 0)
			{
				MG_LOG_ERROR("[CharacterAnimData] action '{}' needs 'clip'.",
					GetTagActName(action.Tags[0]));
				return;
			}

			auto itRootMotion = element.find("rootMotion");
			if (itRootMotion != element.end() && itRootMotion->is_object())
			{
				const bool APPLY = itRootMotion->value("apply", true);

				action.RootMotion.bApplyRootMotion = APPLY;
				action.RootMotion.bApplyTranslationX = itRootMotion->value("x", APPLY);
				action.RootMotion.bApplyTranslationY = itRootMotion->value("y", APPLY);
				action.RootMotion.bApplyTranslationZ = itRootMotion->value("z", APPLY);
				action.RootMotion.bApplyRotationYaw = itRootMotion->value("yaw", APPLY);
			}

			auto itNotifies = element.find("notifies");
			if (itNotifies != element.end() && itNotifies->is_array())
			{
				action.Notifies.reserve(itNotifies->size());

				for (const json& notifyElement : *itNotifies)
				{
					AnimNotifyData notify;
					if (LoadNotify(notifyElement, notify) == false)
						return;

					action.Notifies.push_back(std::move(notify));
				}
			}

			auto itSpeed = element.find("speed");
			if (itSpeed != element.end()) 
			{
				action.Speed = itSpeed->value("speed", action.Speed);
			}

			auto itOffset = element.find("offset");
			if (itOffset != element.end() && itOffset->is_array() && itOffset->size() >= 2)
			{
				action.StartOffset = (*itOffset)[0].get<float>();
				action.EndOffset = (*itOffset)[1].get<float>();
			}

			m_actions.push_back(std::move(action));
		}
	}
	catch (const json::exception& e)
	{
		MG_LOG_ERROR("[CharacterAnimData] parse failed : {}", e.what());
		m_locomotions.clear();
		m_actions.clear();
		return;
	}

	m_bValid = true;
}
