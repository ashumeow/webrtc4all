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
#include "NetTransport.h"
#include "Utils.h"

#include <string>

#define kFuncSetDomain  "SetDomain"
#define kFuncStart  "Start"
#define kFuncStop  "Stop"
#define kFuncSendTo  "SendTo"
#define kFuncSetCallbackFuncName  "setCallbackFuncName"
#define kPropLocalIP  "localIP"
#define kPropLocalPort  "localPort"
#define kPropDefaultDestAddr  "defaultDestAddr"
#define kPropDefaultDestPort  "defaultDestPort"
#define kPropVersion			"version"
#define kPropOpaque  "opaque"

extern NPNetscapeFuncs* BrowserFuncs;
extern const char* kPluginVersion;

NPClass NetTransportClass = {
	NP_CLASS_STRUCT_VERSION,
	NetTransport::Allocate,
	NetTransport::Deallocate,
	NetTransport::Invalidate,
	NetTransport::HasMethod,
	NetTransport::Invoke,
	NetTransport::InvokeDefault,
	NetTransport::HasProperty,
	NetTransport::GetProperty,
	NetTransport::SetProperty,
	NetTransport::RemoveProperty,
	NetTransport::Enumeration,
	NetTransport::Construct,
};


NetTransport::NetTransport(NPP instance)
: _NPObject(instance),
_NetTransport(),
m_Opaque(NULL),
m_CallbackFuncName(NULL)
{
}

NetTransport::~NetTransport()
{
	NP_MEMFREE(m_CallbackFuncName);
	// FIXME: issue on Safari when the browser is refreshed as the opaque object is freed without 'refCount' decrement
	if(m_BrowserType != BrowserType_Safari){
		NP_OBJECT_RELEASE(m_Opaque);
	}
}

bool NetTransport::SetDomain(NPString domain)
{
	char* domainNZ = tsk_strndup(domain.UTF8Characters, domain.UTF8Length);
	bool ret = false;
	if(domainNZ){
		if((ret = _NetTransport::_SetDomain(domainNZ))){
		}
		TSK_FREE(domainNZ);
	}
	return ret;
}

bool NetTransport::Start()
{
	return _NetTransport::_Start(_NetTransport::DgramCb);
}

bool NetTransport::Stop()
{
	return _NetTransport::_Stop();
}

bool NetTransport::SendTo(NPString msg, NPString addr, int port)
{
	char* addrNZ = tsk_strndup(addr.UTF8Characters, addr.UTF8Length);
	bool ret = _NetTransport::_SendTo(msg.UTF8Characters, msg.UTF8Length, addrNZ, port);
	TSK_FREE(addrNZ);
	return ret;
}

void NetTransport::Invalidate(NPObject *npobj)
{
}

bool NetTransport::Construct(NPObject *npobj, const NPVariant *args, uint32_t argCount, NPVariant *result)
{
	return false;
}

NPObject* NetTransport::Allocate(NPP instance, NPClass* npclass)
{
	return (NPObject*)(new NetTransport(instance));
}

void NetTransport::Deallocate(NPObject* obj)
{
	delete (NetTransport*)obj;
}

bool NetTransport::HasMethod(NPObject* obj, NPIdentifier methodName)
{
	char* name = BrowserFuncs->utf8fromidentifier(methodName);
	bool ret_val = !strcmp(name, kFuncSendTo) ||
		!strcmp(name, kFuncStart) ||
		!strcmp(name, kFuncStop) ||
		!strcmp(name, kFuncSetDomain) ||
		!strcmp(name, kFuncSetCallbackFuncName);
	BrowserFuncs->memfree(name);
	return ret_val;
}

bool NetTransport::InvokeDefault(NPObject* obj, const NPVariant* args, uint32_t argCount, NPVariant* result)
{
	return true;
}

bool NetTransport::Invoke(NPObject* obj, NPIdentifier methodName,
						  const NPVariant* args, uint32_t argCount,
						  NPVariant* result) 
{
	NetTransport *This = (NetTransport*)obj;
	char* name = BrowserFuncs->utf8fromidentifier(methodName);
	bool ret_val = false;
	if (!name) {
		return ret_val;
	}

	if(!strcmp(name, kFuncSendTo)){
		if((argCount < 3) || !NPVARIANT_IS_STRING(args[0]) || !NPVARIANT_IS_STRING(args[1]) || (!NPVARIANT_IS_INT32(args[2]) && !NPVARIANT_IS_DOUBLE(args[2]))){
			BrowserFuncs->setexception(obj, "Invalid arguments");
		}
		else{
			const int32_t port = (int32_t)(NPVARIANT_IS_INT32(args[2]) ? NPVARIANT_TO_INT32(args[2]) : NPVARIANT_TO_DOUBLE(args[2]));
			ret_val = This->SendTo(NPVARIANT_TO_STRING(args[0]), NPVARIANT_TO_STRING(args[1]), port);
		}
	}
	else if(!strcmp(name, kFuncStart)){
		ret_val = This->Start();
	}
	else if(!strcmp(name, kFuncStop)){
		ret_val = This->Stop();
	}
	else if(!strcmp(name, kFuncSetDomain)){
		if((argCount < 1) || !NPVARIANT_IS_STRING(args[0])){
			BrowserFuncs->setexception(obj, "Invalid arguments");
		}
		else{
			ret_val = This->SetDomain(NPVARIANT_TO_STRING(args[0]));
		}
	}
	else if(!strcmp(name, kFuncSetCallbackFuncName)){
		if((argCount < 1) || !NPVARIANT_IS_STRING(args[0])){
			BrowserFuncs->setexception(obj, "Invalid arguments");
		}
		else{
			NP_MEMFREE(This->m_CallbackFuncName);
			This->m_CallbackFuncName = (char*)Utils::MemDup(args[0].value.stringValue.UTF8Characters, args[0].value.stringValue.UTF8Length);
			ret_val = (This->m_CallbackFuncName != NULL);
		}
	}
	else{
		// BrowserFuncs->setexception(obj, "Unknown method");
	}

	BrowserFuncs->memfree(name);
	return ret_val;
}

bool NetTransport::HasProperty(NPObject* obj, NPIdentifier propertyName) 
{
	char* name = BrowserFuncs->utf8fromidentifier(propertyName);
	bool ret_val = !strcmp(name, kPropDefaultDestAddr) ||
		!strcmp(name, kPropDefaultDestPort) ||
		!strcmp(name, kPropLocalIP) ||
		!strcmp(name, kPropLocalPort) ||
		!strcmp(name, kPropVersion) ||
		!strcmp(name, kPropOpaque);
	BrowserFuncs->memfree(name);
	return ret_val;
}

bool NetTransport::GetProperty(NPObject* obj, NPIdentifier propertyName, NPVariant* result) 
{
	NetTransport *This = (NetTransport*)obj;
	char* name = BrowserFuncs->utf8fromidentifier(propertyName);
	bool ret_val = false;

	if(!strcmp(name, kPropDefaultDestAddr)){
		if(This->m_DefaultDestAddr){
			char* _str = (char*)Utils::MemDup(This->m_DefaultDestAddr, tsk_strlen(This->m_DefaultDestAddr));
			STRINGN_TO_NPVARIANT(_str, tsk_strlen(_str), *result);
		}
		ret_val = true;
	}
	else if(!strcmp(name, kPropDefaultDestPort)){
		INT32_TO_NPVARIANT(This->m_DefaultDestPort, *result);
		ret_val = true;
	}
	else if(!strcmp(name, kPropLocalIP)){
		if(This->m_pTransport && This->m_pTransport->master){
			char* _str = (char*)Utils::MemDup(This->m_pTransport->master->ip, tsk_strlen(This->m_pTransport->master->ip));
			STRINGN_TO_NPVARIANT(_str, tsk_strlen(_str), *result);
		}
		ret_val = true;
	}
	else if(!strcmp(name, kPropLocalPort)){
		if(This->m_pTransport && This->m_pTransport->master){
			INT32_TO_NPVARIANT(This->m_pTransport->master->port, *result);
		}
		else{
			INT32_TO_NPVARIANT(0, *result);
		}
		ret_val = true;
	}
	else if(!strcmp(name, kPropVersion)){
		STRINGZ_TO_NPVARIANT(kPluginVersion, *result);
		ret_val = true;
	}
	else if(!strcmp(name, kPropOpaque)){
		OBJECT_TO_NPVARIANT(This->m_Opaque, *result);
		ret_val = true;
	}
	else{
		// BrowserFuncs->setexception(obj, "Unknown property");
	}

	BrowserFuncs->memfree(name);

	return ret_val;
}

bool NetTransport::SetProperty(NPObject *npobj, NPIdentifier propertyName, const NPVariant *value)
{
	NetTransport *This = (NetTransport*)npobj;
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
	BrowserFuncs->memfree(name);

	return ret_val;
}

bool NetTransport::RemoveProperty(NPObject *npobj, NPIdentifier propertyName)
{
	return false;
}

bool NetTransport::Enumeration(NPObject *npobj, NPIdentifier **value, uint32_t *count)
{
	return false;
}

LONGLONG NetTransport::GetWindowHandle()
{
	return _NPObject::GetWindowHandle();
}

void NetTransport::DgramCbFire(const NetTransportEvent* e)
{
	if(!m_CallbackFuncName){
		TSK_DEBUG_ERROR("No callback function name defined");
		return;
	}

	NPVariant retval, args[3];
	
	NPIdentifier js_callback = BrowserFuncs->getstringidentifier(m_CallbackFuncName);

	NPObject* window = NULL;
	BrowserFuncs->getvalue(m_npp, NPNVWindowNPObject, &window);

	uint32_t arg_count = 3;
	VOID_TO_NPVARIANT(retval);
	OBJECT_TO_NPVARIANT(m_Opaque, args[0]);
	INT32_TO_NPVARIANT(e->GetType(), args[1]);
	STRINGN_TO_NPVARIANT((const char*)e->GetData(), e->GetDataLen(), args[2]);
	
	BrowserFuncs->invoke(m_npp,
		window,
		js_callback,
		args,
		arg_count,
		&retval);
	BrowserFuncs->releasevariantvalue(&retval);
	BrowserFuncs->releaseobject(window);
}
