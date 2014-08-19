/*
* Copyright (C) 2012 Doubango Telecom <http://www.doubango.org>
*
* Contact: Mamadou Diop <diopmamadou(at)doubango[dot]org>
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
#include "UtilsIE.h"


extern char* g_sNullTerminated ;
extern UINT g_iNullTerminated;
extern CRITICAL_SECTION g_CS;

static BOOL CALLBACK _UtilsEnumWindowsProc( __in  HWND hWnd, __in  LPARAM lParam) ;

_bstr_t Utils::ToBSTR(const char* notNullTerminated, UINT size)
{
	EnterCriticalSection(&g_CS);
	if (g_iNullTerminated < (size + 1)) {
		if (!(g_sNullTerminated = (char*)realloc(g_sNullTerminated, (size + 1)))) g_iNullTerminated = 0;
		else g_iNullTerminated = (size + 1);
	}

	if (g_iNullTerminated) {
		memcpy(g_sNullTerminated, notNullTerminated, size);
		g_sNullTerminated[size] = '\0';
	}

	_bstr_t oData(g_sNullTerminated);

	LeaveCriticalSection(&g_CS);

	return oData;
}

BSTR Utils::SysAllocStringBytes(LPCSTR psz)
{
	if (psz) {
		return _bstr_t(psz).Detach();
	}
	return NULL;
}


void* _UtilsMemAlloc(unsigned n)
{
	return VirtualAlloc(NULL, n, MEM_RESERVE|MEM_COMMIT, PAGE_READWRITE);
}

void _UtilsMemFree(void** mem)
{
    if (mem && *mem) {
		VirtualFree(*mem, 0, MEM_RELEASE);
        *mem = NULL;
    }
}

void* _UtilsMemReAlloc(void* mem, unsigned n)
{
	void* _mem = n > 0 ? _UtilsMemAlloc(n) : NULL;
	if (_mem && mem) {
		MEMORY_BASIC_INFORMATION pbi;
		VirtualQuery(mem, &pbi, sizeof(pbi));
		unsigned sizeToCopy = TSK_MIN((unsigned)pbi.RegionSize, n);
		memcpy(_mem, mem, sizeToCopy);
	}
	_UtilsMemFree(&mem);
	return _mem;
}
