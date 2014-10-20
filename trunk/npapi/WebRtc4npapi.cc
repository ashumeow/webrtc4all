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
#include "WebRtc4npapi.h"
#include "PeerConnection.h"
#include "NetTransport.h"
#include "Utils.h"
#include "SessionDescription.h"
#include "../common/_Utils.h"

#include <string>

#define kFuncCreatePeerConnection  "createPeerConnection"
#define kFuncCreateSessionDescription "createSessionDescription"
#define kFuncCreateNetTransport "createNetTransport"
#define kFuncRunningApps "runningApps"
#define kFuncStartDebug "startDebug"
#define kFuncStopDebug "stopDebug"

#define kPropSupportsPeerConnection "supportsPeerConnection"
#define kPropSupportsSessionDescription "supportsSessionDescription"
#define kPropSupportsNetTransport "supportsNetTransport"
#define kPropVersion			"version"
#define kPropWindowHandle		"windowHandle"
#define kPropHidden			"hidden"
#define kPropFps				"fps"
#define kPropMaxVideoSize		"maxVideoSize"
#define kPropMaxBandwidthUp		"maxBandwidthUp"
#define kPropMaxBandwidthDown	"maxBandwidthDown"
#define kPropZeroArtifacts		"zeroArtifacts"

extern NPClass PeerConnectionClass;
extern NPClass NetTransportClass;
extern NPClass SessionDescriptionClass;

extern NPNetscapeFuncs* BrowserFuncs;
extern const char* kPluginVersion;

NPClass WebRtc4npapiClass = {
	NP_CLASS_STRUCT_VERSION,
	WebRtc4npapi::Allocate,
	WebRtc4npapi::Deallocate,
	WebRtc4npapi::Invalidate,
	WebRtc4npapi::HasMethod,
	WebRtc4npapi::Invoke,
	WebRtc4npapi::InvokeDefault,
	WebRtc4npapi::HasProperty,
	WebRtc4npapi::GetProperty,
	WebRtc4npapi::SetProperty,
	WebRtc4npapi::RemoveProperty,
	WebRtc4npapi::NPEnumeration,
	WebRtc4npapi::Construct,
};

void WebRtc4npapi::Invalidate(NPObject *npobj)
{
}

bool WebRtc4npapi::RemoveProperty(NPObject *npobj, NPIdentifier name)
{
	return false;
}

bool WebRtc4npapi::Construct(NPObject *npobj, const NPVariant *args, uint32_t argCount, NPVariant *result)
{
	return false;
}

WebRtc4npapi::WebRtc4npapi(NPP instance)
: _NPObject(instance)
, _PluginInstance()
, m_bHidden(false)
{
	TSK_DEBUG_INFO("WebRtc4npapi::WebRtc4all");
	_Utils::Initialize();
}

WebRtc4npapi::~WebRtc4npapi()
{
}

NPObject* WebRtc4npapi::Allocate(NPP instance, NPClass* npclass)
{
	return (NPObject*)(new WebRtc4npapi(instance));
}

void WebRtc4npapi::Deallocate(NPObject* obj)
{
	delete (WebRtc4npapi*)obj;
}

bool WebRtc4npapi::HasMethod(NPObject* obj, NPIdentifier methodName)
{
	WebRtc4npapi *This = dynamic_cast<WebRtc4npapi*>((WebRtc4npapi*)obj);
	char* name = BrowserFuncs->utf8fromidentifier(methodName);
	bool ret_val = !strcmp(name, kFuncCreatePeerConnection) ||
		!strcmp(name, kFuncCreateNetTransport) ||
		!strcmp(name, kFuncCreateSessionDescription) ||
		!strcmp(name, kFuncRunningApps) ||
		!strcmp(name, kFuncStartDebug) ||
		!strcmp(name, kFuncStopDebug)		
		;
	BrowserFuncs->memfree(name);
	return ret_val;
}

bool WebRtc4npapi::InvokeDefault(NPObject* obj, const NPVariant* args, uint32_t argCount, NPVariant* result)
{
	return true;
}

bool WebRtc4npapi::Invoke(NPObject* obj, NPIdentifier methodName,
						  const NPVariant* args, uint32_t argCount,
						  NPVariant* result) 
{
	WebRtc4npapi *This = dynamic_cast<WebRtc4npapi*>((WebRtc4npapi*)obj);
	char* name = BrowserFuncs->utf8fromidentifier(methodName);
	bool ret_val = false;
	if (!name) {
		return ret_val;
	}

	if(!strcmp(name, kFuncCreatePeerConnection)){
		PeerConnection* peerObj = (PeerConnection*)BrowserFuncs->createobject(This->m_npp, &PeerConnectionClass);
		if(peerObj){
			if((ret_val = peerObj->SetWindow(This->m_pWindow, true))){
				OBJECT_TO_NPVARIANT(peerObj, *result);
			}
			else{
				NP_OBJECT_RELEASE(peerObj);
			}
		}
	}
	else if(!strcmp(name, kFuncCreateNetTransport)){
		NetTransport* netTransportObj = (NetTransport*)BrowserFuncs->createobject(This->m_npp, &NetTransportClass);
		if(netTransportObj){
			if((ret_val = netTransportObj->SetWindow(This->m_pWindow, true))){
				OBJECT_TO_NPVARIANT(netTransportObj, *result);
			}
			else{
				NP_OBJECT_RELEASE(netTransportObj);
			}
		}
	}
	else if (!strcmp(name, kFuncCreateSessionDescription)) {
		SessionDescription* sdpObj = (SessionDescription*)BrowserFuncs->createobject(This->m_npp, &SessionDescriptionClass);
		if(sdpObj){
			if(argCount > 0 && NPVARIANT_IS_STRING(args[0])){
				if((ret_val = sdpObj->Init(args[0].value.stringValue.UTF8Characters, args[0].value.stringValue.UTF8Length))){
					OBJECT_TO_NPVARIANT(sdpObj, *result);
				}
				else{
					NP_OBJECT_RELEASE(sdpObj);
				}
			}
			else{
				ret_val = true;
				OBJECT_TO_NPVARIANT(sdpObj, *result);
			}
		}
	}
	else if (!strcmp(name, kFuncRunningApps)) {
		_ActiveApps* activeApps = _Utils::GetActiveApps();
		if (activeApps && activeApps->GetDataPtr() && activeApps->GetDataSize()) {
			char* npMem = (char*)Utils::MemDup(activeApps->GetDataPtr(), activeApps->GetDataSize());
			if (npMem) {
				STRINGZ_TO_NPVARIANT(npMem, *result);
				ret_val = true;
			}
		}
		if (activeApps) {
			delete activeApps;
		}
	}
	else if (!strcmp(name, kFuncStartDebug)) {
		VOID_TO_NPVARIANT(*result);
		ret_val = _Utils::StartDebug();
	}
	else if (!strcmp(name, kFuncStopDebug)) {
		VOID_TO_NPVARIANT(*result);
		ret_val = _Utils::StopDebug();
	}
	else {
		// BrowserFuncs->setexception(obj, "Unknown method");
	}

	BrowserFuncs->memfree(name);
	return ret_val;
}

bool WebRtc4npapi::HasProperty(NPObject* obj, NPIdentifier propertyName) 
{
	char* name = BrowserFuncs->utf8fromidentifier(propertyName);
	bool ret_val = !strcmp(name, kPropSupportsPeerConnection) ||
			!strcmp(name, kPropSupportsSessionDescription) ||
			!strcmp(name, kPropVersion) ||
			!strcmp(name, kPropSupportsNetTransport) ||
			!strcmp(name, kPropWindowHandle) ||
			!strcmp(name, kPropHidden) ||
			!strcmp(name, kPropFps) ||
			!strcmp(name, kPropMaxVideoSize) ||
			!strcmp(name, kPropMaxBandwidthUp) ||
			!strcmp(name, kPropMaxBandwidthDown) ||
			!strcmp(name, kPropZeroArtifacts)
			;
	BrowserFuncs->memfree(name);
	return ret_val;
}

bool WebRtc4npapi::GetProperty(NPObject* obj, NPIdentifier propertyName, NPVariant* result) 
{
	WebRtc4npapi *This = dynamic_cast<WebRtc4npapi*>((WebRtc4npapi*)obj);
	char* name = BrowserFuncs->utf8fromidentifier(propertyName);
	bool ret_val = false;
	if (!name) {
		return ret_val;
	}

	if (!strcmp(name, kPropSupportsPeerConnection)) {
		BOOLEAN_TO_NPVARIANT(true, *result);
		ret_val = true;
	}
	else if (!strcmp(name, kPropSupportsSessionDescription)) {
		BOOLEAN_TO_NPVARIANT(true, *result);
		ret_val = true;
	}
	else if (!strcmp(name, kPropSupportsNetTransport)) {
		BOOLEAN_TO_NPVARIANT(true, *result);
		ret_val = true;
	}
	else if (!strcmp(name, kPropVersion)) {
		char* _str = (char*)_Utils::MemDup(THIS_VERSION, tsk_strlen(THIS_VERSION));
		STRINGZ_TO_NPVARIANT(_str, *result);
		ret_val = true;
	}
	else if (!strcmp(name, kPropWindowHandle)) {
		DOUBLE_TO_NPVARIANT((double)This->GetWindowHandle(), *result);
		ret_val = true;
	}
	else if (!strcmp(name, kPropHidden)) {
		BOOLEAN_TO_NPVARIANT(This->m_bHidden, *result);
		ret_val = true;
	}
	else if (!strcmp(name, kPropFps)) {
		DOUBLE_TO_NPVARIANT(_PluginInstance::GetFps(), *result);
		ret_val = true;
	}
	else if (!strcmp(name, kPropMaxVideoSize)) {
		char* _str = (char*)_Utils::MemDup(_PluginInstance::GetMaxVideoSize(), tsk_strlen(THIS_VERSION));
		STRINGZ_TO_NPVARIANT(_str, *result);
		ret_val = true;
	}
	else if (!strcmp(name, kPropMaxBandwidthUp)) {
		DOUBLE_TO_NPVARIANT(_PluginInstance::GetMaxBandwidthUp(), *result);
		ret_val = true;
	}
	else if (!strcmp(name, kPropMaxBandwidthDown)) {
		DOUBLE_TO_NPVARIANT(_PluginInstance::GetMaxBandwidthDown(), *result);
		ret_val = true;
	}
	else if (!strcmp(name, kPropZeroArtifacts)) {
		BOOLEAN_TO_NPVARIANT(_PluginInstance::GetZeroArtifacts(), *result);
		ret_val = true;
	}

	BrowserFuncs->memfree(name);
	return ret_val;
}

bool WebRtc4npapi::SetProperty(NPObject *npobj, NPIdentifier propertyName, const NPVariant *value)
{
	WebRtc4npapi *This = dynamic_cast<WebRtc4npapi*>((WebRtc4npapi*)npobj);
	char* name = BrowserFuncs->utf8fromidentifier(propertyName);
	bool ret_val = false;

	if (!strcmp(name, kPropFps)) {
		if ((NPVARIANT_IS_DOUBLE(*value) || NPVARIANT_IS_INT32(*value))) {
			ret_val = _PluginInstance::SetFps((long)(NPVARIANT_IS_DOUBLE(*value) ? value->value.doubleValue : value->value.intValue));
		}
	}
	else if (!strcmp(name, kPropMaxVideoSize)) {
		char* maxSize = tsk_strndup((const char*)value->value.stringValue.UTF8Characters, value->value.stringValue.UTF8Length);
		ret_val = _PluginInstance::SetMaxVideoSize(maxSize);
		TSK_FREE(maxSize);
	}
	else if (!strcmp(name, kPropMaxBandwidthUp)) {
		if ((NPVARIANT_IS_DOUBLE(*value) || NPVARIANT_IS_INT32(*value))) {
			ret_val = _PluginInstance::SetMaxBandwidthUp((long)(NPVARIANT_IS_DOUBLE(*value) ? value->value.doubleValue : value->value.intValue));
		}
	}
	else if (!strcmp(name, kPropMaxBandwidthDown)) {
		if ((NPVARIANT_IS_DOUBLE(*value) || NPVARIANT_IS_INT32(*value))) {
			ret_val = _PluginInstance::SetMaxBandwidthDown((long)(NPVARIANT_IS_DOUBLE(*value) ? value->value.doubleValue : value->value.intValue));
		}
	}
	else if (!strcmp(name, kPropZeroArtifacts)) {
		if ((NPVARIANT_IS_BOOLEAN(*value))) {
			ret_val = _PluginInstance::SetZeroArtifacts(value->value.boolValue);
		}
	}
	else if (!strcmp(name, kPropHidden)) {
		if (NPVARIANT_IS_BOOLEAN(*value)) {
			This->m_bHidden = value->value.boolValue;
#if W4A_UNDER_WINDOWS
			if (This->GetWindowHandle()) {
				ShowWindow(((HWND)This->GetWindowHandle()), This->m_bHidden ? SW_HIDE : SW_SHOW);
			}
#elif W4A_UNDER_APPLE
			// FIXME: not implemented yet
#endif
			ret_val = true;
		}
	}
	
	BrowserFuncs->memfree(name);

	return ret_val;
}

bool WebRtc4npapi::NPEnumeration(NPObject *npobj, NPIdentifier **value, uint32_t *count)
{
	return false;
}
