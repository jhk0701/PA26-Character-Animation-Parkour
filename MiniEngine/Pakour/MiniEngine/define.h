#pragma once
#include <directxtk/SimpleMath.h>

#define WINDOW_STYLE WS_OVERLAPPEDWINDOW

#define SINGLETON(Type)\
public:\
	static Type* GetInstance()\
	{\
		static Type instance;\
		return &instance;\
	}\
protected:\
	Type();\
	~Type();
