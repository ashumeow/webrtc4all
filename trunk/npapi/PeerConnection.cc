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
#include "PeerConnection.h"
#include "SessionDescription.h"
#include "Display.h"
#include "Utils.h"

#define kFuncInit  "Init"
#define kFuncClose  "close"
#define kFuncCreateOffer  "createOffer"
#define kFuncCreateAnswer  "createAnswer"
#define kFuncStartIce  "startIce"
#define kFuncSetLocalDescription  "setLocalDescription"
#define kFuncSetRemoteDescription  "setRemoteDescription"
#define kFuncSetCallbackFuncName  "setCallbackFuncName"
#define kFuncAttachDisplays  "attachDisplays"
#define kPropLocalDescription  "localDescription"
#define kPropRemoteDescription  "remoteDescription"
#define kPropReadyState  "readyState"
#define kPropIceState  "iceState"
#define kPropRemoteVideo  "remoteVideo"
#define kPropLocalVideo  "localVideo"
#define kPropOpaque  "opaque"
#define kPropVersion  "version"
#define kPropFullScreen "fullScreen"

extern NPNetscapeFuncs* BrowserFuncs;
extern const char* kPluginVersion;
extern NPClass SessionDescriptionClass;

NPClass PeerConnectionClass = {
  NP_CLASS_STRUCT_VERSION,
  PeerConnection::Allocate,
  PeerConnection::Deallocate,
  NULL,
  PeerConnection::HasMethod,
  PeerConnection::Invoke,
  PeerConnection::InvokeDefault,
  PeerConnection::HasProperty,
  PeerConnection::GetProperty,
  PeerConnection::SetProperty,
  NULL,
  PeerConnection::Enumeration,
  NULL,
};

PeerConnection::PeerConnection(NPP instance)
    : _NPObject(instance),
	_PeerConnection(m_BrowserType),
	m_Opaque(NULL),
	m_CallbackFuncName(NULL)
{
}

PeerConnection::~PeerConnection()
{	
	TSK_FREE(m_CallbackFuncName);
	// FIXME: issue on Safari when the browser is refreshed as the opaque object is freed without 'refCount' decrement
	if(m_BrowserType != BrowserType_Safari){
		NP_OBJECT_RELEASE(m_Opaque);
	}
}

LONGLONG PeerConnection::GetWindowHandle()
{
	return _NPObject::GetWindowHandle();
}

void PeerConnection::IceCallbackFire(const PeerConnectionEvent* e)
{
	if(!m_CallbackFuncName){
		TSK_DEBUG_ERROR("No callback function defined");
		return;
	}

	NPVariant retval, args[4];
	
	NPIdentifier js_callback = BrowserFuncs->getstringidentifier(m_CallbackFuncName);

	NPObject* window = NULL;
	BrowserFuncs->getvalue(m_npp, NPNVWindowNPObject, &window);

	uint32_t arg_count = 4;
	VOID_TO_NPVARIANT(retval);
	OBJECT_TO_NPVARIANT(m_Opaque, args[0]);
	STRINGN_TO_NPVARIANT(e->GetMedia(), tsk_strlen(e->GetMedia()), args[1]);
	STRINGN_TO_NPVARIANT(e->GetCandidate(), tsk_strlen(e->GetCandidate()), args[2]);
	BOOLEAN_TO_NPVARIANT(e->GetMoreToFollow(), args[3]);

	BrowserFuncs->invoke(m_npp,
		window,
		js_callback,
		args,
		arg_count,
		&retval);
	BrowserFuncs->releasevariantvalue(&retval);
	BrowserFuncs->releaseobject(window);
}

NPObject* PeerConnection::Allocate(NPP instance, NPClass* npclass)
{
  return (NPObject*)(new PeerConnection(instance));
}

void PeerConnection::Deallocate(NPObject* obj)
{
  delete (PeerConnection*)obj;
}

bool PeerConnection::HasMethod(NPObject* obj, NPIdentifier methodName)
{
  char* name = BrowserFuncs->utf8fromidentifier(methodName);
  bool ret_val = !strcmp(name, kFuncInit) ||
		!strcmp(name, kFuncClose) ||
		!strcmp(name, kFuncStartIce) ||
		!strcmp(name, kFuncCreateOffer) ||
		!strcmp(name, kFuncCreateAnswer) ||
		!strcmp(name, kFuncSetLocalDescription) ||
		!strcmp(name, kFuncSetRemoteDescription) ||
		!strcmp(name, kFuncSetCallbackFuncName) ||
		!strcmp(name, kFuncAttachDisplays);
  BrowserFuncs->memfree(name);
  return ret_val;
}

bool PeerConnection::InvokeDefault(NPObject* obj, const NPVariant* args, uint32_t argCount, NPVariant* result)
{
  return true;
}

bool PeerConnection::Invoke(NPObject* obj, NPIdentifier methodName,
                     const NPVariant* args, uint32_t argCount,
                     NPVariant* result) 
{
  PeerConnection *This = (PeerConnection*)obj;
  char* name = BrowserFuncs->utf8fromidentifier(methodName);
  bool ret_val = false;
  if (!name) {
    return ret_val;
  }

  if(!strcmp(name, kFuncInit)){
	  if(argCount < 1 || !NPVARIANT_IS_STRING(args[0])){
			BrowserFuncs->setexception(obj, "Invalid argument");
	  }
	  else{
		  ret_val = This->_Init(args[0].value.stringValue.UTF8Characters, args[0].value.stringValue.UTF8Length);
	  }
  }
  else if(!strcmp(name, kFuncClose)){
	ret_val = This->Close();
  }
  else if(!strcmp(name, kFuncStartIce)){
	  int IceOptions = (argCount > 0 && NPVARIANT_IS_INT32(args[0])) ? NPVARIANT_TO_INT32(args[0]) : 0;
	  ret_val = This->StartIce(IceOptions);
  }
  else if(!strcmp(name, kFuncCreateOffer) || !strcmp(name, kFuncCreateAnswer)){
	  bool is_offer = !strcmp(name, kFuncCreateOffer);
	  bool has_audio = (argCount > 0 && NPVARIANT_IS_BOOLEAN(args[0]) && NPVARIANT_TO_BOOLEAN(args[0]));
	  bool has_video = (argCount > 1 && NPVARIANT_IS_BOOLEAN(args[1]) && NPVARIANT_TO_BOOLEAN(args[1]));
	  char* sdpStr = NULL;
	  int sdpStrLen;
	  ret_val = This->CreateLo(has_audio, has_video, &sdpStr, &sdpStrLen, is_offer);
	  if(ret_val){
		  SessionDescription* sdpObj = (SessionDescription*)BrowserFuncs->createobject(This->m_npp, &SessionDescriptionClass);
		  if((ret_val = sdpObj->Init(sdpStr, sdpStrLen))){
			OBJECT_TO_NPVARIANT(sdpObj, *result);
		  }
		  else if(sdpObj){
			  NP_OBJECT_RELEASE(sdpObj);
		  }
	  }
	  TSK_FREE(sdpStr);
  }
  else if(!strcmp(name, kFuncSetLocalDescription)){
	  if(argCount < 2 || (!NPVARIANT_IS_INT32(args[0]) && !NPVARIANT_IS_DOUBLE(args[0])) || !NPVARIANT_IS_OBJECT(args[1]) || !dynamic_cast<SessionDescription*>((SessionDescription*)args[1].value.objectValue)){
		BrowserFuncs->setexception(obj, "Invalid argument");
	  }
	  else{
		  const int action = (int32_t)(NPVARIANT_IS_INT32(args[0]) ? NPVARIANT_TO_INT32(args[0]) : NPVARIANT_TO_DOUBLE(args[0]));
		  ret_val = This->SetLocalDescription(action, dynamic_cast<SessionDescription*>((SessionDescription*)args[1].value.objectValue));
	  }
  }
  else if(!strcmp(name, kFuncSetRemoteDescription)){
	  if(argCount < 2 || (!NPVARIANT_IS_INT32(args[0]) && !NPVARIANT_IS_DOUBLE(args[0])) || !NPVARIANT_IS_OBJECT(args[1]) || !dynamic_cast<SessionDescription*>((SessionDescription*)args[1].value.objectValue)){
		BrowserFuncs->setexception(obj, "Invalid argument");
	  }
	  else{
		  const int action = (int32_t)(NPVARIANT_IS_INT32(args[0]) ? NPVARIANT_TO_INT32(args[0]) : NPVARIANT_TO_DOUBLE(args[0]));
		  ret_val = This->SetRemoteDescription(action, dynamic_cast<SessionDescription*>((SessionDescription*)args[1].value.objectValue));
	  }
  }
  else if(!strcmp(name, kFuncSetCallbackFuncName)){
		if((argCount < 1) || !NPVARIANT_IS_STRING(args[0])){
			BrowserFuncs->setexception(obj, "Invalid arguments");
		}
		else{
			TSK_FREE(This->m_CallbackFuncName);
			This->m_CallbackFuncName = tsk_strndup(args[0].value.stringValue.UTF8Characters, args[0].value.stringValue.UTF8Length);
			ret_val = (This->m_CallbackFuncName != NULL);
		}
	}
  else if(!strcmp(name, kFuncAttachDisplays)){
	  if(argCount < 2 || (!NPVARIANT_IS_DOUBLE(args[0]) && !NPVARIANT_IS_INT32(args[0])) || (!NPVARIANT_IS_DOUBLE(args[1]) && !NPVARIANT_IS_INT32(args[1]))){
		BrowserFuncs->setexception(obj, "Invalid argument");
	  }
	  else{
		  const LONGLONG local = (LONGLONG)(NPVARIANT_IS_DOUBLE(args[0]) ? args[0].value.doubleValue : args[0].value.intValue);
		  const LONGLONG remote = (LONGLONG)(NPVARIANT_IS_DOUBLE(args[1]) ? args[1].value.doubleValue : args[1].value.intValue);
		  ret_val = This->SetDisplays(local, remote);
	  }
  }
  else{
	  // BrowserFuncs->setexception(obj, "Unknown method");
  }
 
  BrowserFuncs->memfree(name);
  return ret_val;
}

bool PeerConnection::HasProperty(NPObject* obj, NPIdentifier propertyName) 
{
	char* name = BrowserFuncs->utf8fromidentifier(propertyName);
	bool ret_val = !strcmp(name, kPropLocalDescription) ||
		!strcmp(name, kPropRemoteDescription) ||
		!strcmp(name, kPropOpaque) ||
		!strcmp(name, kPropLocalVideo) ||
		!strcmp(name, kPropRemoteVideo);
	BrowserFuncs->memfree(name);
	return ret_val;
}

bool PeerConnection::GetProperty(NPObject* obj, NPIdentifier propertyName, NPVariant* result) 
{
	PeerConnection *This = (PeerConnection*)obj;
	char* name = BrowserFuncs->utf8fromidentifier(propertyName);
	bool ret_val = false;
	
	if(!strcmp(name, kPropLocalDescription)){
		ret_val = true;
		if(This->mSdpLocal){
			char* _sdp = tsdp_message_tostring(This->mSdpLocal);
			char* _str = (char*)Utils::MemDup(_sdp, tsk_strlen(_sdp));
			STRINGN_TO_NPVARIANT(_str, tsk_strlen(_str), *result);
			TSK_FREE(_sdp);
		}
	}
	else if(!strcmp(name, kPropRemoteDescription)){
		ret_val = true;
		if(This->mSdpRemote){
			char* _sdp = tsdp_message_tostring(This->mSdpRemote);
			char* _str = (char*)Utils::MemDup(_sdp, tsk_strlen(_sdp));
			STRINGN_TO_NPVARIANT(_str, tsk_strlen(_str), *result);
			TSK_FREE(_sdp);
		}
	}
	else if(!strcmp(name, kPropOpaque)){
		OBJECT_TO_NPVARIANT(This->m_Opaque, *result);
		ret_val = true;
	}
	else if(!strcmp(name, kPropLocalVideo)){
		DOUBLE_TO_NPVARIANT((double)This->mLocalVideo, *result);
		ret_val = true;
	}
	else if(!strcmp(name, kPropRemoteVideo)){
		DOUBLE_TO_NPVARIANT((double)This->mRemoteVideo, *result);
		ret_val = true;
	}
	else{
		// BrowserFuncs->setexception(obj, "Unknown property");
	}
	
	BrowserFuncs->memfree(name);
	
	return ret_val;
}

bool PeerConnection::SetProperty(NPObject *npobj, NPIdentifier propertyName, const NPVariant *value)
{
	PeerConnection *This = dynamic_cast<PeerConnection*>((PeerConnection*)npobj);
	char* name = BrowserFuncs->utf8fromidentifier(propertyName);
	bool ret_val = false;

	if(!strcmp(name, kPropOpaque)){
		if(!NPVARIANT_IS_OBJECT(*value)){
			BrowserFuncs->setexception(npobj, "Invalid parameter type");
		}
		else{
			NP_OBJECT_RELEASE(This->m_Opaque);
			This->m_Opaque = BrowserFuncs->retainobject(value->value.objectValue);
			ret_val = true;
		}
	}
	else if(!strcmp(name, kPropLocalVideo)){
		if((!NPVARIANT_IS_DOUBLE(*value) && !NPVARIANT_IS_INT32(*value))){
			BrowserFuncs->setexception(npobj, "Invalid argument");
		}
		else{
		  ret_val = This->SetDisplayLocal((LONGLONG)(NPVARIANT_IS_DOUBLE(*value) ? value->value.doubleValue : value->value.intValue));
		}
	}
	else if(!strcmp(name, kPropRemoteVideo)){
		if((!NPVARIANT_IS_DOUBLE(*value) && !NPVARIANT_IS_INT32(*value))){
			BrowserFuncs->setexception(npobj, "Invalid argument");
		}
		else{
		  ret_val = This->SetDisplayRemote((LONGLONG)(NPVARIANT_IS_DOUBLE(*value) ? value->value.doubleValue : value->value.intValue));
		}
	}
	BrowserFuncs->memfree(name);

	return ret_val;
}

bool PeerConnection::Enumeration(NPObject *npobj, NPIdentifier **value, uint32_t *count)
{
	return false;
}

