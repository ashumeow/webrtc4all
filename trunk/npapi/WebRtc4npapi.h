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
#ifndef _WEBRTC4NPAPI_WEBRTC4ALL_H_
#define _WEBRTC4NPAPI_WEBRTC4ALL_H_

#include "../common/_Config.h"

class WebRtc4npapi: public NPObject {
public:
	WebRtc4npapi(NPP instance);
	virtual ~WebRtc4npapi();
	bool SetWindow(NPWindow* pWindow);

public:
	static NPObject* Allocate(NPP instance, NPClass* npclass);
	static void Deallocate(NPObject* obj);
	static bool HasMethod(NPObject* obj, NPIdentifier methodName);
	static bool InvokeDefault(NPObject* obj, const NPVariant* args,
							uint32_t argCount, NPVariant* result);
	static bool Invoke(NPObject* obj, NPIdentifier methodName,
					 const NPVariant* args, uint32_t argCount,
					 NPVariant* result);
	static bool HasProperty(NPObject* obj, NPIdentifier propertyName);
	static bool GetProperty(NPObject* obj, NPIdentifier propertyName,
						  NPVariant* result);
	static bool SetProperty(NPObject *npobj, NPIdentifier name, const NPVariant *value);
	static bool RemoveProperty(NPObject *npobj, NPIdentifier name);
	static bool NPEnumeration(NPObject *npobj, NPIdentifier **value, 
						uint32_t *count);
	static void Invalidate(NPObject *npobj);
	static bool Construct(NPObject *npobj, const NPVariant *args, uint32_t argCount, NPVariant *result);

private:
	NPWindow* m_pWindow;
#if W4A_UNDER_WINDOWS
	WNDPROC m_pWinProc;
#endif
	NPP m_npp;
};

#endif /* _WEBRTC4NPAPI_WEBRTC4ALL_H_ */
