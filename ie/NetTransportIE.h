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

#include "../common/_NetTransport.h"

#include "webrtc4ie_i.h"
#include "_INetTransportEvents_CP.h"

#if defined(_WIN32_WCE) && !defined(_CE_DCOM) && !defined(_CE_ALLOW_SINGLE_THREADED_OBJECTS_IN_MTA)
#error "Single-threaded COM objects are not properly supported on Windows CE platform, such as the Windows Mobile platforms that do not include full DCOM support. Define _CE_ALLOW_SINGLE_THREADED_OBJECTS_IN_MTA to force ATL to support creating single-thread COM object's and allow use of it's single-threaded COM object implementations. The threading model in your rgs file was set to 'Free' as that is the only threading model supported in non DCOM Windows CE platforms."
#endif


// CNetTransport

class ATL_NO_VTABLE CNetTransport :
	public _NetTransport,
	public CComObjectRootEx<CComSingleThreadModel>,
	public CComCoClass<CNetTransport, &CLSID_NetTransport>,
	public ISupportErrorInfo,
	public IConnectionPointContainerImpl<CNetTransport>,
	public CProxy_INetTransportEvents<CNetTransport>,
	public IObjectWithSiteImpl<CNetTransport>,
	public IProvideClassInfo2Impl<&CLSID_NetTransport, &__uuidof(_INetTransportEvents), &LIBID_webrtc4ieLib>,
	public IDispatchImpl<INetTransport, &IID_INetTransport, &LIBID_webrtc4ieLib, /*wMajor =*/ 1, /*wMinor =*/ 0>,
	public IObjectSafetyImpl<CNetTransport, INTERFACESAFE_FOR_UNTRUSTED_CALLER | INTERFACESAFE_FOR_UNTRUSTED_DATA>
{
public:
	CNetTransport();
	virtual ~CNetTransport();

DECLARE_REGISTRY_RESOURCEID(IDR_NETTRANSPORT)


BEGIN_COM_MAP(CNetTransport)
	COM_INTERFACE_ENTRY(INetTransport)
	COM_INTERFACE_ENTRY(IDispatch)
	COM_INTERFACE_ENTRY(ISupportErrorInfo)
	COM_INTERFACE_ENTRY(IConnectionPointContainer)
	COM_INTERFACE_ENTRY(IObjectWithSite)
	COM_INTERFACE_ENTRY(IProvideClassInfo)
    COM_INTERFACE_ENTRY(IProvideClassInfo2)
	COM_INTERFACE_ENTRY(IObjectSafety)
END_COM_MAP()

BEGIN_CONNECTION_POINT_MAP(CNetTransport)
	CONNECTION_POINT_ENTRY(__uuidof(_INetTransportEvents))
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
	STDMETHOD(SetDomain)(BSTR domain);
	STDMETHOD(Start)(LONGLONG looper);
	STDMETHOD(SendTo)(BSTR msg, BSTR addr, USHORT port);
	STDMETHOD(Stop)(void);
	STDMETHOD(get_localIP)(BSTR* pVal);
	STDMETHOD(get_localPort)(USHORT* pVal);
	STDMETHOD(get_defaultDestAddr)(BSTR* pVal);
	STDMETHOD(get_defaultDestPort)(USHORT* pVal);
	STDMETHOD(get_version)(BSTR* pVal);

	STDMETHOD(StartDebug)(void);
	STDMETHOD(StopDebug)(void);

public:
	virtual LONGLONG GetWindowHandle();
	virtual void DgramCbFire(const NetTransportEvent* e);

private:
	HWND mLooperHandle;
	WNDPROC mLooperProc;
};

OBJECT_ENTRY_AUTO(__uuidof(NetTransport), CNetTransport)
