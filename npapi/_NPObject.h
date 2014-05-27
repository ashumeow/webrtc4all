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
#if !defined(_WEBRTC4NPAPI_NPOBJECT_H_)
#define _WEBRTC4NPAPI_NPOBJECT_H_

#include "../common/_Config.h"

class _NPObject: public NPObject {
public:
	_NPObject(NPP instance);
	virtual ~_NPObject();
	bool SetWindow(NPWindow* pWindow, bool bSubClass);
	LONGLONG GetWindowHandle();
    
	bool StartDebug();
	bool StopDebug();

protected:
	NPWindow* m_pWindow;
#if W4A_UNDER_WINDOWS
	WNDPROC m_pWinProc;
#endif
	NPP m_npp;
	BrowserType_t m_BrowserType;
};

#endif /* _WEBRTC4NPAPI_NPOBJECT_H_ */
