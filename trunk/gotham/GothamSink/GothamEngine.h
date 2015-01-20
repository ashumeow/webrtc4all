#pragma once

#include <Windows.h>

#include "GothamTypes.h"

class GmEngine
{
public:
	static HRESULT Initialize();
	static HRESULT DeInitialize();
private:
	static bool s_bInitialized;
};
