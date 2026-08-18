#pragma once
#include <array>
#include "UI/UIBase.h"
#include "Asset/AnimClip.h"

namespace MiniEngine { class SkinnedMesh; }
using namespace MiniEngine;

// 기존 SkinnedMesh .mini 를 열어 클립을 관리하는 패널.
// 재베이크 없이 클립 이름/순서/삭제를 고치고, 애니 소스를 리타게팅해 덧붙인 뒤 다시 쓴다.
class UIMiniEditPanel : public UIBase
{
public:
	void Construct() override;

protected:
	void DrawUI() override;

private:
	void RebuildNameBufs();

	char m_miniPath[1024]{};    // 대상 .mini 경로(로드 + 저장 대상)
	char m_appendAnims[1024]{}; // 덧붙일 애니 소스(';' 구분)
	int  m_upAxis = 0;          // append 용 BakeUpAxis

	std::shared_ptr<SkinnedMesh>      m_mesh;     // 로드된 원본(정점/인덱스/스켈레톤 제공)
	std::vector<AnimClip>             m_clips;    // 편집 중인 클립 목록(저장 시 이걸 쓴다)
	std::vector<std::array<char, 64>> m_nameBufs; // 클립 이름 입력 버퍼(m_clips 와 1:1)

	std::string m_msg;
	bool m_dirty = false;
};
