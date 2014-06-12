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
#define kFuncCreateOfferEx  "createOfferEx"
#define kFuncCreateAnswerEx  "createAnswerEx"
#define kFuncStartIce  "startIce"
#define kFuncSetLocalDescription  "setLocalDescription"
#define kFuncSetRemoteDescription  "setRemoteDescription"
#define kFuncSetCallbackFuncName  "setCallbackFuncName"
#define kFuncSetRfc5168CallbackFuncName  "setRfc5168CallbackFuncName"
#define kFuncSetBfcpCallbackFuncName  "setBfcpCallbackFuncName"
#define kFuncAttachDisplays  "attachDisplays"
#define kFuncStartMedia  "startMedia"
#define kFuncProcessContent  "processContent"
#define kFuncSendDTMF  "sendDTMF"
#define kPropLocalDescription  "localDescription"
#define kPropRemoteDescription  "remoteDescription"
#define kPropReadyState  "readyState"
#define kPropIceState  "iceState"
#define kPropRemoteVideo  "remoteVideo"
#define kPropLocalVideo  "localVideo"
#define kPropLocalScreencast  "localScreencast"
#define kPropSrcScreencast  "srcScreencast"
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
    m_Instance(instance),
	m_Opaque(NULL),
	m_CallbackFuncName(NULL),
	m_Rfc5168CallbackFuncName(NULL),
	m_BfcpCallbackFuncName(NULL)
{
}

PeerConnection::~PeerConnection()
{	
	TSK_FREE(m_CallbackFuncName);
	TSK_FREE(m_Rfc5168CallbackFuncName);
	TSK_FREE(m_BfcpCallbackFuncName);
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
		TSK_DEBUG_ERROR("No callback function defined. Did you forget to call '%s'", kFuncSetCallbackFuncName);
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

void PeerConnection::Rfc5168CallbackFire(const char* commandStr)
{
	if (!m_Rfc5168CallbackFuncName) {
		TSK_DEBUG_ERROR("No callback function defined. Did you forget to call '%s'", kFuncSetRfc5168CallbackFuncName);
		return;
	}

	NPVariant retval, args[4];
	
	NPIdentifier js_callback = BrowserFuncs->getstringidentifier(m_Rfc5168CallbackFuncName);
    char* npCommandStr = (char*)_Utils::MemDup(commandStr, (unsigned)tsk_strlen(commandStr)); // use memalloc()

	NPObject* window = NULL;
	BrowserFuncs->getvalue(m_npp, NPNVWindowNPObject, &window);

	uint32_t arg_count = 2;
	VOID_TO_NPVARIANT(retval);
	OBJECT_TO_NPVARIANT(m_Opaque, args[0]);
	STRINGZ_TO_NPVARIANT(npCommandStr, args[1]);

	BrowserFuncs->invoke(m_npp,
		window,
		js_callback,
		args,
		arg_count,
		&retval);
	BrowserFuncs->releasevariantvalue(&retval);
	BrowserFuncs->releaseobject(window);
    _Utils::MemFree((void**)&npCommandStr);
}

void PeerConnection::BfcpCallbackFire(const char* descStr)
{
	if (!m_BfcpCallbackFuncName) {
		TSK_DEBUG_ERROR("No callback function defined. Did you forget to call '%s'", kFuncSetRfc5168CallbackFuncName);
		return;
	}

	NPVariant retval, args[4];
	
	NPIdentifier js_callback = BrowserFuncs->getstringidentifier(m_BfcpCallbackFuncName);
    char* npDescStr = (char*)_Utils::MemDup(descStr, (unsigned)tsk_strlen(descStr)); // use memalloc()

	NPObject* window = NULL;
	BrowserFuncs->getvalue(m_npp, NPNVWindowNPObject, &window);

	uint32_t arg_count = 2;
	VOID_TO_NPVARIANT(retval);
	OBJECT_TO_NPVARIANT(m_Opaque, args[0]);
	STRINGZ_TO_NPVARIANT(npDescStr, args[1]);

	BrowserFuncs->invoke(m_npp,
		window,
		js_callback,
		args,
		arg_count,
		&retval);
	BrowserFuncs->releasevariantvalue(&retval);
	BrowserFuncs->releaseobject(window);
    _Utils::MemFree((void**)&npDescStr);
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
		!strcmp(name, kFuncCreateOfferEx) ||
		!strcmp(name, kFuncCreateAnswerEx) ||
		!strcmp(name, kFuncSetLocalDescription) ||
		!strcmp(name, kFuncSetRemoteDescription) ||
		!strcmp(name, kFuncSetCallbackFuncName) ||
		!strcmp(name, kFuncSetRfc5168CallbackFuncName) ||
		!strcmp(name, kFuncSetBfcpCallbackFuncName) ||
		!strcmp(name, kFuncAttachDisplays) ||
		!strcmp(name, kFuncStartMedia) ||
		!strcmp(name, kFuncProcessContent) ||
		!strcmp(name, kFuncSendDTMF)
		;
		
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
  else if(!strcmp(name, kFuncCreateOffer) || !strcmp(name, kFuncCreateAnswer) || !strcmp(name, kFuncCreateOfferEx) || !strcmp(name, kFuncCreateAnswerEx)){
	  bool is_offer = !strcmp(name, kFuncCreateOffer) || !strcmp(name, kFuncCreateOfferEx);
	  bool has_audio = (argCount > 0 && NPVARIANT_IS_BOOLEAN(args[0]) && NPVARIANT_TO_BOOLEAN(args[0]));
	  bool has_video = (argCount > 1 && NPVARIANT_IS_BOOLEAN(args[1]) && NPVARIANT_TO_BOOLEAN(args[1]));
	  bool has_bfcpvideo = (argCount > 2 && NPVARIANT_IS_BOOLEAN(args[2]) && NPVARIANT_TO_BOOLEAN(args[2]));
	  char* sdpStr = NULL;
	  int sdpStrLen;
	  ret_val = This->CreateLo(has_audio, has_video, has_bfcpvideo, &sdpStr, &sdpStrLen, is_offer);
	  if (ret_val) {
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
  else if(!strcmp(name, kFuncSetRfc5168CallbackFuncName)){
		if((argCount < 1) || !NPVARIANT_IS_STRING(args[0])){
			BrowserFuncs->setexception(obj, "Invalid arguments");
		}
		else{
			TSK_FREE(This->m_Rfc5168CallbackFuncName);
			This->m_Rfc5168CallbackFuncName = tsk_strndup(args[0].value.stringValue.UTF8Characters, args[0].value.stringValue.UTF8Length);
			ret_val = (This->m_Rfc5168CallbackFuncName != NULL);
		}
  }
  else if(!strcmp(name, kFuncSetBfcpCallbackFuncName)){
		if((argCount < 1) || !NPVARIANT_IS_STRING(args[0])){
			BrowserFuncs->setexception(obj, "Invalid arguments");
		}
		else{
			TSK_FREE(This->m_BfcpCallbackFuncName);
			This->m_BfcpCallbackFuncName = tsk_strndup(args[0].value.stringValue.UTF8Characters, args[0].value.stringValue.UTF8Length);
			ret_val = (This->m_BfcpCallbackFuncName != NULL);
		}
  }
  else if(!strcmp(name, kFuncAttachDisplays)){ // deprecated
	  if(argCount < 2 || (!NPVARIANT_IS_DOUBLE(args[0]) && !NPVARIANT_IS_INT32(args[0])) || (!NPVARIANT_IS_DOUBLE(args[1]) && !NPVARIANT_IS_INT32(args[1]))){
		BrowserFuncs->setexception(obj, "Invalid argument");
	  }
	  else{
		  const LONGLONG local = (LONGLONG)(NPVARIANT_IS_DOUBLE(args[0]) ? args[0].value.doubleValue : args[0].value.intValue);
		  const LONGLONG remote = (LONGLONG)(NPVARIANT_IS_DOUBLE(args[1]) ? args[1].value.doubleValue : args[1].value.intValue);
		  ret_val = This->SetDisplays(local, remote, 0, 0);
	  }
  }
  else if(!strcmp(name, kFuncStartMedia)){
	  ret_val = This->StartMedia();
  }
  else if(!strcmp(name, kFuncProcessContent)){
		// ProcessContent(const char* req_name, const char* content_type, const void* content_ptr, int content_size)
	  if(argCount < 4 || (!NPVARIANT_IS_STRING(args[0]) || !NPVARIANT_IS_STRING(args[1]) || !NPVARIANT_IS_STRING(args[2]) || (!NPVARIANT_IS_INT32(args[3]) && !NPVARIANT_IS_DOUBLE(args[3])))){
		BrowserFuncs->setexception(obj, "Invalid argument");
	  }
	  else{
		char* req_name = tsk_strndup((const char*)args[0].value.stringValue.UTF8Characters, args[0].value.stringValue.UTF8Length);
		char* content_type = tsk_strndup((const char*)args[1].value.stringValue.UTF8Characters, args[1].value.stringValue.UTF8Length);
		char* content_ptr = tsk_strndup((const char*)args[2].value.stringValue.UTF8Characters, args[2].value.stringValue.UTF8Length);
		int content_size = (int)(NPVARIANT_IS_DOUBLE(args[3]) ? args[3].value.doubleValue : args[3].value.intValue);
		ret_val = This->ProcessContent(req_name, content_type, content_ptr, content_size);
		TSK_FREE(req_name);
		TSK_FREE(content_type);
		TSK_FREE(content_ptr);
	  }
  }
  else if(!strcmp(name, kFuncSendDTMF)) {
	  if (argCount == 1 && (NPVARIANT_IS_STRING(args[0]) || NPVARIANT_IS_INT32(args[0]) || NPVARIANT_IS_DOUBLE(args[0]))) {
		  uint8_t digit = (uint8_t)(NPVARIANT_IS_STRING(args[0]) 
				? args[0].value.stringValue.UTF8Characters[0] 
				: (NPVARIANT_IS_INT32(args[0]) ? args[0].value.intValue : args[0].value.doubleValue));
		  ret_val = This->SendDTMF(digit);
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
		!strcmp(name, kPropRemoteVideo) ||
		!strcmp(name, kPropLocalScreencast) ||
		!strcmp(name, kPropSrcScreencast) ||
		!strcmp(name, kPropIceState);
	BrowserFuncs->memfree(name);
	return ret_val;
}

bool PeerConnection::GetProperty(NPObject* obj, NPIdentifier propertyName, NPVariant* result) 
{
	PeerConnection *This = (PeerConnection*)obj;
	char* name = BrowserFuncs->utf8fromidentifier(propertyName);
	bool ret_val = false;
	
	if (!strcmp(name, kPropLocalDescription)) {
		ret_val = true;
		if (This->mSdpLocal) {
			char* _sdp = tsdp_message_tostring(This->mSdpLocal);
			char* npSdpStr = (char*)_Utils::MemDup(_sdp, (unsigned)tsk_strlen(_sdp));
			STRINGN_TO_NPVARIANT(npSdpStr, tsk_strlen(npSdpStr), *result);
			TSK_FREE(_sdp);
		}
	}
	else if (!strcmp(name, kPropRemoteDescription)) {
		ret_val = true;
		if (This->mSdpRemote) {
			char* _sdp = tsdp_message_tostring(This->mSdpRemote);
			char* npSdpStr = (char*)_Utils::MemDup(_sdp, (unsigned)tsk_strlen(_sdp));
			STRINGN_TO_NPVARIANT(npSdpStr, tsk_strlen(npSdpStr), *result);
			TSK_FREE(_sdp);
		}
	}
	else if (!strcmp(name, kPropOpaque)) {
		OBJECT_TO_NPVARIANT(This->m_Opaque, *result);
		ret_val = true;
	}
	else if (!strcmp(name, kPropLocalVideo)) {
		DOUBLE_TO_NPVARIANT((double)This->mLocalVideo, *result);
		ret_val = true;
	}
	else if (!strcmp(name, kPropRemoteVideo)) {
		DOUBLE_TO_NPVARIANT((double)This->mRemoteVideo, *result);
		ret_val = true;
	}
	else if (!strcmp(name, kPropLocalScreencast)) {
		DOUBLE_TO_NPVARIANT((double)This->mLocalScreencast, *result);
		ret_val = true;
	}
	else if (!strcmp(name, kPropSrcScreencast)) {
		DOUBLE_TO_NPVARIANT((double)This->mSrcScreencast, *result);
		ret_val = true;
	}
	else if (!strcmp(name, kPropIceState)) {
		INT32_TO_NPVARIANT((int32_t)This->mIceState, *result);
		ret_val = true;
	}
	
	else {
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
		  ret_val = This->SetDisplayLocalVideo((LONGLONG)(NPVARIANT_IS_DOUBLE(*value) ? value->value.doubleValue : value->value.intValue));
		}
	}
	else if(!strcmp(name, kPropRemoteVideo)){
		if ((!NPVARIANT_IS_DOUBLE(*value) && !NPVARIANT_IS_INT32(*value))) {
			BrowserFuncs->setexception(npobj, "Invalid argument");
		}
		else {
		  ret_val = This->SetDisplayRemoteVideo((LONGLONG)(NPVARIANT_IS_DOUBLE(*value) ? value->value.doubleValue : value->value.intValue));
		}
	}
	else if(!strcmp(name, kPropLocalScreencast)){
		if ((!NPVARIANT_IS_DOUBLE(*value) && !NPVARIANT_IS_INT32(*value))) {
			BrowserFuncs->setexception(npobj, "Invalid argument");
		}
		else {
			ret_val = This->SetDisplayLocalScreencast((LONGLONG)(NPVARIANT_IS_DOUBLE(*value) ? value->value.doubleValue : value->value.intValue));
		}
	}
	else if(!strcmp(name, kPropSrcScreencast)){
		if ((!NPVARIANT_IS_DOUBLE(*value) && !NPVARIANT_IS_INT32(*value))) {
			BrowserFuncs->setexception(npobj, "Invalid argument");
		}
		else {
			ret_val = This->SetDisplaySrcScreencast((LONGLONG)(NPVARIANT_IS_DOUBLE(*value) ? value->value.doubleValue : value->value.intValue));
		}
	}
	BrowserFuncs->memfree(name);

	return ret_val;
}

bool PeerConnection::Enumeration(NPObject *npobj, NPIdentifier **value, uint32_t *count)
{
	return false;
}

