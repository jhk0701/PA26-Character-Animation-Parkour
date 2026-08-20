#pragma once
#include "UI/UIBase.h"

using namespace MiniEngine;

// 원본 모델(.fbx/.gltf/.obj) → .mini 베이크 패널.
class UIBakePanel : public UIBase
{
public:
	void Construct() override;

protected:
	void DrawUI() override;

private:
	// 입력 버퍼(UTF-8). Construct 에서 Assets 폴더 기준 기본값을 채운다.
	char m_src[1024]{};      // 원본 모델 경로
	char m_out[1024]{};      // 출력 .mini 경로
	char m_anims[4096]{};    // 추가 애니 소스(';' 구분, 동일/호환 스켈레톤)
	char m_animsDir[1024]{}; // 추가 애니 소스 폴더(비재귀 일괄 등록)

	int m_upAxis      = 0; // BakeUpAxis      : Auto / Y-up / Z-up / -Z-up
	int m_forwardAxis = 0; // BakeForwardAxis : Auto / +Z / -Z / +X / -X

	// 마지막 베이크 결과
	std::string m_lastMsg;
	std::string m_lastUp;      // detectedUp      (정규화 전 실측 up)
	std::string m_lastForward; // detectedForward (up 보정 후 실측 정면)
	bool m_lastOk      = false;
	bool m_lastSkinned = false;
	int  m_lastVerts   = 0;
	int  m_lastIdx     = 0;
	int  m_lastBones   = 0;
	int  m_lastClips   = 0;
};
