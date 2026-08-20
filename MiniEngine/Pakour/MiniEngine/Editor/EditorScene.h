#pragma once
#include "Scene/Scene.h"

namespace MiniEngine
{
	class EditorScene : public Scene
	{
	public:
		void Construct(ID3D11Device* _device, ID3D11DeviceContext* _context) override;
	};
}
