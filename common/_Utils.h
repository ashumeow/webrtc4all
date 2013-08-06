/*
* Copyright (C) 2012 Doubango Telecom <http://www.doubango.org>
*
* This file is part of Open Source webrtc4all project <http://code.google.com/p/webrtc4all/>
*
* webrtc4all is free software: you can redistribute it and/or modify
* it under the terms of the GNU General Public License as publishd by
* the Free Software Foundation, either version 3 of the License, or
* (at your option) any later version.
*	
* webrtc4all is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*	
* You should have received a copy of the GNU General Public License
* along with webrtc4ie.
*/
#ifndef _WEBRTC4ALL_UTILS_H_
#define _WEBRTC4ALL_UTILS_H_

#include "_Config.h"

#if W4A_IE
#	include <comutil.h>
#endif

class _Utils
{
public:
	_Utils();
	virtual ~_Utils();
	
	static void Initialize(void);
	static bool StartDebug(void);
	static bool StopDebug(void);
	static const char* GetCurrentDirectoryPath();
	static LRESULT CALLBACK WndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
};


#endif /* _WEBRTC4ALL_UTILS_H_ */
