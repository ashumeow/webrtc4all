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
#include "SessionDescription.h"
#include "Utils.h"

#include <string>

#define kFuncToSdp  "toSdp"
#define kFuncInit  "Init"
#define kFuncAddCandidate "addCandidate"
#define kPropVersion			"version"

extern NPNetscapeFuncs* BrowserFuncs;
extern const char* kPluginVersion;

NPClass SessionDescriptionClass = {
	NP_CLASS_STRUCT_VERSION,
	SessionDescription::Allocate,
	SessionDescription::Deallocate,
	NULL,
	SessionDescription::HasMethod,
	SessionDescription::Invoke,
	SessionDescription::InvokeDefault,
	SessionDescription::HasProperty,
	SessionDescription::GetProperty,
	NULL,
	NULL,
	SessionDescription::NPEnumeration,
	NULL,
};

SessionDescription::SessionDescription(NPP instance)
: _NPObject(instance),
_SessionDescription()
{
}

SessionDescription::~SessionDescription()
{
}

NPObject* SessionDescription::Allocate(NPP instance, NPClass* npclass)
{
	return (NPObject*)(new SessionDescription(instance));
}

void SessionDescription::Deallocate(NPObject* obj)
{
	delete (SessionDescription*)obj;
}

bool SessionDescription::HasMethod(NPObject* obj, NPIdentifier methodName)
{
	char* name = BrowserFuncs->utf8fromidentifier(methodName);
	bool ret_val = !strcmp(name, kFuncToSdp) ||
		!strcmp(name, kFuncInit) ||
		!strcmp(name, kFuncAddCandidate) ||
		!strcmp(name, kPropVersion);
	BrowserFuncs->memfree(name);
	return ret_val;
}

bool SessionDescription::InvokeDefault(NPObject* obj, const NPVariant* args, uint32_t argCount, NPVariant* result)
{
	return true;
}

bool SessionDescription::Invoke(NPObject* obj, NPIdentifier methodName,
						  const NPVariant* args, uint32_t argCount,
						  NPVariant* result) 
{
	SessionDescription *This = (SessionDescription*)obj;
	char* name = BrowserFuncs->utf8fromidentifier(methodName);
	bool ret_val = false;
	if (!name) {
		return ret_val;
	}
	
	if(!strcmp(name, kFuncInit)){
		if((argCount < 1) || !NPVARIANT_IS_STRING(args[0])){
			TSK_DEBUG_ERROR("Invalid arguments");
		}
		else{
			ret_val = This->Init(args[0].value.stringValue.UTF8Characters, args[0].value.stringValue.UTF8Length);
		}
	}
	else if(!strcmp(name, kFuncToSdp)){
		if(This->m_Sdp){
			char* sdpStr = tsdp_message_tostring(This->m_Sdp);
			char* npSdpStr = (char*)Utils::MemDup(sdpStr, tsk_strlen(sdpStr));
			STRINGN_TO_NPVARIANT(npSdpStr, tsk_strlen(npSdpStr), *result);
			TSK_FREE(sdpStr);
		}
		ret_val = true;
	}
	else if(!strcmp(name, kFuncAddCandidate)){
		if((argCount < 2) || !NPVARIANT_IS_STRING(args[0]) || !NPVARIANT_IS_STRING(args[1])){
			TSK_DEBUG_ERROR("Invalid arguments");
		}
		else{
			char* mediaNZ = tsk_strndup(args[0].value.stringValue.UTF8Characters, args[0].value.stringValue.UTF8Length);
			char* candidateNZ = tsk_strndup(args[1].value.stringValue.UTF8Characters, args[1].value.stringValue.UTF8Length);
			ret_val = This->AddCandidate(mediaNZ, candidateNZ);
			TSK_FREE(mediaNZ);
			TSK_FREE(candidateNZ);
		}
	}
	else{
		// BrowserFuncs->setexception(obj, "Unknown method");
	}
	
	BrowserFuncs->memfree(name);
	return ret_val;
}

bool SessionDescription::HasProperty(NPObject* obj, NPIdentifier propertyName) 
{
	return false;
}

bool SessionDescription::GetProperty(NPObject* obj, NPIdentifier propertyName, NPVariant* result) 
{
	return false;
}

bool SessionDescription::NPEnumeration(NPObject *npobj, NPIdentifier **value, uint32_t *count)
{
	return false;
}
