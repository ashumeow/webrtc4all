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
#ifndef _WEBRTC4NPAPI_PEERCONNECTION_H_
#define _WEBRTC4NPAPI_PEERCONNECTION_H_

#include "../common/_PeerConnection.h"
#include "_NPObject.h"

class PeerConnection: public _NPObject, public _PeerConnection {
public:
  PeerConnection(NPP instance);
  virtual ~PeerConnection();

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
  static bool SetProperty(NPObject *npobj, NPIdentifier propertyName, 
						 const NPVariant *value);
  static bool Enumeration(NPObject *npobj, NPIdentifier **value, 
                          uint32_t *count);

public:
	virtual LONGLONG GetWindowHandle();
	virtual void IceCallbackFire(const PeerConnectionEvent* e);
	virtual void Rfc5168CallbackFire(const char* commandStr);
	virtual void BfcpCallbackFire(const char* descStr);
    NPP GetInstance() { return m_Instance; }

private:
	NPObject* m_Opaque;
    NPP m_Instance;
	char* m_CallbackFuncName;
	char* m_Rfc5168CallbackFuncName;
	char* m_BfcpCallbackFuncName;
};

#endif /* _WEBRTC4NPAPI_PEERCONNECTION_H_ */
