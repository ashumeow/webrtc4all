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
#include "Display.h"
#include "../common/_Utils.h"

#include "tinydshow/DSDisplay.h"

#include <string>

#define kFuncSetFullScreen  "setFullScreen"
#define kPropHWnd			"hWnd"
#define kPropSupportsHidden			"supportsHidden"
#define kPropHidden			"hidden"
#define kPropVersion			"version"


extern NPNetscapeFuncs* BrowserFuncs;
extern const char* kPluginVersion;

NPClass DisplayClass = {
	NP_CLASS_STRUCT_VERSION,
	Display::Allocate,
	Display::Deallocate,
	NULL,
	Display::HasMethod,
	Display::Invoke,
	Display::InvokeDefault,
	Display::HasProperty,
	Display::GetProperty,
	Display::SetProperty,
	NULL,
	Display::Enumeration,
	NULL,
};

Display::Display(NPP instance)
: _NPObject(instance)
{
	TSK_DEBUG_INFO("Display::Display");
	_Utils::Initialize();
}

Display::~Display()
{
}

NPObject* Display::Allocate(NPP instance, NPClass* npclass)
{
	return (NPObject*)(new Display(instance));
}

void Display::Deallocate(NPObject* obj)
{
	delete (Display*)obj;
}

bool Display::HasMethod(NPObject* obj, NPIdentifier methodName)
{
	char* name = BrowserFuncs->utf8fromidentifier(methodName);
	bool ret_val = !strcmp(name, kFuncSetFullScreen);
	BrowserFuncs->memfree(name);
	return ret_val;
}

bool Display::InvokeDefault(NPObject* obj, const NPVariant* args, uint32_t argCount, NPVariant* result)
{
	return true;
}

bool Display::Invoke(NPObject* obj, NPIdentifier methodName,
						  const NPVariant* args, uint32_t argCount,
						  NPVariant* result) 
{
	Display *This = (Display*)obj;
	char* name = BrowserFuncs->utf8fromidentifier(methodName);
	bool ret_val = false;
	if (!name) {
		return ret_val;
	}

	if(!strcmp(name, kFuncSetFullScreen)){
		if((argCount < 1) || !NPVARIANT_IS_BOOLEAN(args[0])){
			BrowserFuncs->setexception(obj, "Invalid arguments");
		}
		else{
			if(This->m_pWindow && This->m_pWindow->window){
				static const BOOL __fsTrue = TRUE;
				static const BOOL __fsFalse = FALSE;
				PostMessageA((HWND)This->GetWindowHandle(), WM_FULLSCREEN_SET, reinterpret_cast<WPARAM>(This), reinterpret_cast<LPARAM>((args[0].value.boolValue ? &__fsTrue : &__fsFalse)));
			}
		}
	}

	BrowserFuncs->memfree(name);
	return ret_val;
}

bool Display::HasProperty(NPObject* obj, NPIdentifier propertyName) 
{
	char* name = BrowserFuncs->utf8fromidentifier(propertyName);
	bool ret_val = !strcmp(name, kPropHWnd) ||
		!strcmp(name, kPropSupportsHidden) ||
		!strcmp(name, kPropHidden);
	BrowserFuncs->memfree(name);
	return ret_val;
}

bool Display::GetProperty(NPObject* obj, NPIdentifier propertyName, NPVariant* result) 
{
	Display *This = (Display*)obj;
	char* name = BrowserFuncs->utf8fromidentifier(propertyName);
	bool ret_val = false;
	
	if(!strcmp(name, kPropHWnd)){
		ret_val = true;
		LONGLONG val = (This->m_pWindow ? (LONGLONG)This->m_pWindow->window : 0);
		DOUBLE_TO_NPVARIANT((double)val, *result);
	}
	else if(!strcmp(name, kPropSupportsHidden)){
		ret_val = true;
		BOOLEAN_TO_NPVARIANT(true, *result);
	}
	else{
		// BrowserFuncs->setexception(obj, "Unknown property");
	}	
	
	BrowserFuncs->memfree(name);
	
	return ret_val;
}

bool Display::SetProperty(NPObject *npobj, NPIdentifier propertyName, const NPVariant *value)
{
	Display *This = dynamic_cast<Display*>((Display*)npobj);
	char* name = BrowserFuncs->utf8fromidentifier(propertyName);
	bool ret_val = false;

	if(!strcmp(name, kPropHidden)){
		if(!NPVARIANT_IS_BOOLEAN(*value)){
			BrowserFuncs->setexception(npobj, "Invalid argument");
		}
		else{
			ret_val = true;
			if(This->m_pWindow && This->m_pWindow->window){
				ShowWindow(((HWND)This->m_pWindow->window), value->value.boolValue ? SW_HIDE : SW_SHOW);
			}
		}
	}
	BrowserFuncs->memfree(name);

	return ret_val;
}



bool Display::Enumeration(NPObject *npobj, NPIdentifier **value, uint32_t *count)
{
	return false;
}
