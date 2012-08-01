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

#include "../common/_SessionDescription.h"
#include "../common/_Utils.h"

#include "webrtc4ie_i.h"

#if defined(_WIN32_WCE) && !defined(_CE_DCOM) && !defined(_CE_ALLOW_SINGLE_THREADED_OBJECTS_IN_MTA)
#error "Single-threaded COM objects are not properly supported on Windows CE platform, such as the Windows Mobile platforms that do not include full DCOM support. Define _CE_ALLOW_SINGLE_THREADED_OBJECTS_IN_MTA to force ATL to support creating single-thread COM object's and allow use of it's single-threaded COM object implementations. The threading model in your rgs file was set to 'Free' as that is the only threading model supported in non DCOM Windows CE platforms."
#endif


// CSessionDescription

class ATL_NO_VTABLE CSessionDescription :
	public _SessionDescription,
	public CComObjectRootEx<CComSingleThreadModel>,
	public CComCoClass<CSessionDescription, &CLSID_SessionDescription>,
	public ISupportErrorInfo,
	public IObjectWithSiteImpl<CSessionDescription>,
	public IDispatchImpl<ISessionDescription, &IID_ISessionDescription, &LIBID_webrtc4ieLib, /*wMajor =*/ 1, /*wMinor =*/ 0>,
	public IObjectSafetyImpl<CSessionDescription, INTERFACESAFE_FOR_UNTRUSTED_CALLER | INTERFACESAFE_FOR_UNTRUSTED_DATA>
{
public:
	CSessionDescription();
	virtual ~CSessionDescription();

DECLARE_REGISTRY_RESOURCEID(IDR_SESSIONDESCRIPTION)


BEGIN_COM_MAP(CSessionDescription)
	COM_INTERFACE_ENTRY(ISessionDescription)
	COM_INTERFACE_ENTRY(IDispatch)
	COM_INTERFACE_ENTRY(ISupportErrorInfo)
	COM_INTERFACE_ENTRY(IObjectWithSite)
	COM_INTERFACE_ENTRY(IObjectSafety)
END_COM_MAP()

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

	STDMETHOD(toSdp)(BSTR* sdp);
	STDMETHOD(Init)(BSTR sdp);
	STDMETHOD(addCandidate)(BSTR media, BSTR candidate);
};

OBJECT_ENTRY_AUTO(__uuidof(SessionDescription), CSessionDescription)
