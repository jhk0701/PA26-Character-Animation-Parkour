#pragma once

namespace MiniEngine 
{
	class Animator;
	class ActionClip;
	class SkinnedMesh;
}

struct ActionClipLoadParam 
{
	std::shared_ptr<MiniEngine::Animator> pAnim;
	std::shared_ptr<MiniEngine::SkinnedMesh> pSources;
	std::unordered_map<uint8_t, std::shared_ptr<MiniEngine::ActionClip>>* pMaps;
};

class ActionClipContainer
{
public:
	// TODO : 애니메이션 데이터화 후, 실제로 경로 기반 로드해올 것
	static void LoadActionClips(ActionClipLoadParam& _param);

};