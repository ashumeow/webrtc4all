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

#if W4A_UNDER_WINDOWS
#	include <comutil.h>
#endif

class _ActiveApps
{
public:
	_ActiveApps();
	virtual ~_ActiveApps();
	bool AppendApp(LONGLONG windowID, const void* descPtr, size_t descSize, const void* iconDataPtr, size_t iconDataSize, const void* iconTypePtr, size_t iconTypeSize);
	const void* GetDataPtr() { return m_pDataPtr; }
	size_t GetDataSize() { return m_nDataSize; }
private:
	void* m_pDataPtr;
	size_t m_nDataSize;
};

class _Utils
{
public:
	_Utils();
	virtual ~_Utils();
	
	static void Initialize(void);
	static bool StartDebug(void);
	static bool StopDebug(void);
    static const char* GetCurrentDirectoryPath();
    static bool PostMessage(LONGLONG handle, unsigned msg, void* wParam, void** lParam);
	static LRESULT CALLBACK WndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
    static void* MemAlloc(unsigned n);
    static void* MemDup(const void* mem, unsigned n);
	static void* MemReAlloc(void* mem, unsigned n);
    static void MemFree(void** mem);
	static _ActiveApps* GetActiveApps();
};


#endif /* _WEBRTC4ALL_UTILS_H_ */
