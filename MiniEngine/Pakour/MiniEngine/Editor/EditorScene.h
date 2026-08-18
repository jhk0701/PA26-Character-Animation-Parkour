#pragma once
#include "Scene/Scene.h"

namespace MiniEngine
{
	// 에디터 전용 빈 씬.
	// Editor 구성에서 TestScene(파쿠르 캐릭터/장애물/인식 트리) 대신 올라간다 —
	// 베이크 작업 중 콘텐츠 로직이 같이 돌지 않게 하고, 콘텐츠 초기화 실패가 에디터를 막지 않게 한다.
	class EditorScene : public Scene
	{
	public:
		void Construct(ID3D11Device* _device, ID3D11DeviceContext* _context) override;
	};
}
