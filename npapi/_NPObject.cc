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
#include "_NPObject.h"
#include "../common/_Utils.h"

extern NPNetscapeFuncs* BrowserFuncs;

_NPObject::_NPObject(NPP instance)
: m_npp(instance),
m_pWindow(NULL),
m_pWinProc(NULL),
m_BrowserType(BrowserType_None)
{
	_Utils::Initialize();
	if(strstr(BrowserFuncs->uagent(m_npp), "Safari")){
		m_BrowserType = BrowserType_Safari;
	}
}

_NPObject::~_NPObject()
{
	SetWindow(NULL, false);
}

bool _NPObject::SetWindow(NPWindow* pWindow, bool bSubClass)
{
	if(m_pWindow && m_pWindow->window && m_pWinProc){
		SetWindowLongPtr((HWND)m_pWindow->window, GWL_WNDPROC, (LONG)m_pWinProc);
		m_pWinProc = NULL;
	}

	if((m_pWindow = pWindow) && m_pWindow->window){
		if(bSubClass){
			m_pWinProc = (WNDPROC) SetWindowLongPtr((HWND)m_pWindow->window, GWL_WNDPROC, (LONG)_Utils::WndProc);
			if(!m_pWinProc){
				TSK_DEBUG_ERROR("SetWindowLongPtr() failed with errcode=%d", GetLastError());
				return false;
			}
		}
	}
	
	return true;
}

LONGLONG _NPObject::GetWindowHandle()
{
	return m_pWindow ? (LONGLONG)m_pWindow->window : 0;
}