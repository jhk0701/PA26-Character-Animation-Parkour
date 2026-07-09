#pragma once
#include <windows.h>
#include <string>   
#include "Core/Math.h"

struct ID3D11Device;
struct ID3D11DeviceContext;

namespace MiniEngine 
{
	class Scene;
	class UIManager
	{
		SINGLETON(UIManager)

	public:
		bool Init(HWND _hWnd, ID3D11Device* _device, ID3D11DeviceContext* _context);
		void Shutdown();
		void BuildUI(Scene& _world, const Matrix& _view, const Matrix& _proj);
		void Render();

	private:
		bool m_initialized = false;
	};
}