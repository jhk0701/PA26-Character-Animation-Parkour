#pragma once

#include <memory>
#include <unordered_map>

namespace MiniEngine
{
	class Animator;
	class ActionClip;
	class SkinnedMesh;
}

class CharacterAnimData;

struct ActionClipLoadParam
{
	std::shared_ptr<MiniEngine::Animator> pAnim;
	std::shared_ptr<MiniEngine::SkinnedMesh> pSources;
	std::unordered_map<uint8_t, std::shared_ptr<MiniEngine::ActionClip>>* pMaps;
	const CharacterAnimData* pAnimData;
};

// CharacterAnimData -> BlendClip / ActionClip 팩토리
class ActionClipContainer
{
public:
	static void LoadActionClips(ActionClipLoadParam& _param);
};
