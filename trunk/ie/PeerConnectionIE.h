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
#pragma once
#include "resource.h"       // main symbols

#include "../common/_Utils.h"
#include "../common/_PeerConnection.h"

#include "webrtc4ie_i.h"
#include "_IPeerConnectionEvents_CP.h"


#if defined(_WIN32_WCE) && !defined(_CE_DCOM) && !defined(_CE_ALLOW_SINGLE_THREADED_OBJECTS_IN_MTA)
#error "Single-threaded COM objects are not properly supported on Windows CE platform, such as the Windows Mobile platforms that do not include full DCOM support. Define _CE_ALLOW_SINGLE_THREADED_OBJECTS_IN_MTA to force ATL to support creating single-thread COM object's and allow use of it's single-threaded COM object implementations. The threading model in your rgs file was set to 'Free' as that is the only threading model supported in non DCOM Windows CE platforms."
#endif


// CPeerConnection

class ATL_NO_VTABLE CPeerConnection :
	public _PeerConnection,
	public CComObjectRootEx<CComSingleThreadModel>,
	public CComCoClass<CPeerConnection, &CLSID_PeerConnection>,
	public ISupportErrorInfo,
	public IConnectionPointContainerImpl<CPeerConnection>,
	public CProxy_IPeerConnectionEvents<CPeerConnection>,
	public IObjectWithSiteImpl<CPeerConnection>,
	public IProvideClassInfo2Impl<&CLSID_PeerConnection, &__uuidof(_IPeerConnectionEvents), &LIBID_webrtc4ieLib>,
	public IDispatchImpl<IPeerConnection, &IID_IPeerConnection, &LIBID_webrtc4ieLib, /*wMajor =*/ 1, /*wMinor =*/ 0>,
	public IObjectSafetyImpl<CPeerConnection, INTERFACESAFE_FOR_UNTRUSTED_CALLER | INTERFACESAFE_FOR_UNTRUSTED_DATA>
{
public:
	CPeerConnection();
	virtual ~CPeerConnection();

DECLARE_REGISTRY_RESOURCEID(IDR_PEERCONNECTION)


BEGIN_COM_MAP(CPeerConnection)
	COM_INTERFACE_ENTRY(IPeerConnection)
	COM_INTERFACE_ENTRY(IDispatch)
	COM_INTERFACE_ENTRY(ISupportErrorInfo)
	COM_INTERFACE_ENTRY(IConnectionPointContainer)
	COM_INTERFACE_ENTRY(IObjectWithSite)
	COM_INTERFACE_ENTRY(IProvideClassInfo)
    COM_INTERFACE_ENTRY(IProvideClassInfo2)
	COM_INTERFACE_ENTRY(IObjectSafety)
END_COM_MAP()

BEGIN_CONNECTION_POINT_MAP(CPeerConnection)
	CONNECTION_POINT_ENTRY(__uuidof(_IPeerConnectionEvents))
END_CONNECTION_POINT_MAP()
// ISupportsErrorInfo
	STDMETHOD(InterfaceSupportsErrorInfo)(REFIID riid);


	DECLARE_PROTECT_FINAL_CONSTRUCT()

	HRESULT FinalConstruct()
	{
		return S_OK;
	}

	void FinalRelease()
	{
	}

public:
	STDMETHOD(close)(void);
	STDMETHOD(createOffer)(VARIANT_BOOL has_audio, VARIANT_BOOL has_video, BSTR* sdp);
	STDMETHOD(createAnswer)(VARIANT_BOOL has_audio, VARIANT_BOOL has_video, BSTR* sdp);
	STDMETHOD(startIce)(SHORT IceOptions, LONGLONG looper);
	STDMETHOD(startMedia)();
	STDMETHOD(setLocalDescription)(USHORT action, BSTR desc);
	STDMETHOD(setRemoteDescription)(USHORT action, BSTR desc);
	STDMETHOD(processContent)(BSTR req_name, BSTR content_type, BSTR content_ptr, INT content_size);
	STDMETHOD(get_localDescription)(BSTR* pVal);
	STDMETHOD(get_remoteDescription)(BSTR* pVal);
	STDMETHOD(get_readyState)(USHORT* pVal);
	STDMETHOD(get_iceState)(USHORT* pVal);
	STDMETHOD(get_remoteVideo)(LONGLONG* pVal);
	STDMETHOD(put_remoteVideo)(LONGLONG newVal);
	STDMETHOD(get_localVideo)(LONGLONG* pVal);
	STDMETHOD(put_localVideo)(LONGLONG newVal);
	STDMETHOD(get_version)(BSTR* pVal);
	STDMETHOD(get_fullScreen)(VARIANT_BOOL* pVal);
	STDMETHOD(put_fullScreen)(VARIANT_BOOL newVal);
	
	STDMETHOD(Init)(BSTR configuration);
	STDMETHOD(StartDebug)(void);
	STDMETHOD(StopDebug)(void);

public:
	virtual void IceCallbackFire(const PeerConnectionEvent* e);
	virtual void Rfc5168CallbackFire(const char* commandStr);
	virtual LONGLONG GetWindowHandle();

private:
	HWND mLooperHandle;
	WNDPROC mLooperProc;
};

OBJECT_ENTRY_AUTO(__uuidof(PeerConnection), CPeerConnection)
