#include "pch.h"
#include "Perception/Interface/IPerceptionProcessor.h"

#ifdef MG_DEBUG_UI

static int raycastUsageCnt; // 디버그 UI에 띄울 용도로만 사용
void MiniEngine::AddRaycastUsageCnt() { raycastUsageCnt++; }
void MiniEngine::ResetRaycastUsageCnt() { raycastUsageCnt = 0; }
int MiniEngine::GetRaycastUsageCnt() { return raycastUsageCnt; }

#endif
