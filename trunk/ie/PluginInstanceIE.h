/*
* Copyright (C) 2014 Doubango Telecom <http://www.doubango.org>
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
#include "../common/_PluginInstance.h"

#include "webrtc4ie_i.h"

#if defined(_WIN32_WCE) && !defined(_CE_DCOM) && !defined(_CE_ALLOW_SINGLE_THREADED_OBJECTS_IN_MTA)
#error "Single-threaded COM objects are not properly supported on Windows CE platform, such as the Windows Mobile platforms that do not include full DCOM support. Define _CE_ALLOW_SINGLE_THREADED_OBJECTS_IN_MTA to force ATL to support creating single-thread COM object's and allow use of it's single-threaded COM object implementations. The threading model in your rgs file was set to 'Free' as that is the only threading model supported in non DCOM Windows CE platforms."
#endif


// CPluginInstance
class ATL_NO_VTABLE CPluginInstance :
	public _PluginInstance,
	public CComObjectRootEx<CComSingleThreadModel>,
	public IDispatchImpl<IPluginInstance, &IID_IPluginInstance, &LIBID_webrtc4ieLib, /*wMajor =*/ 1, /*wMinor =*/ 0>,
	public IPersistStreamInitImpl<CPluginInstance>,
	public IOleControlImpl<CPluginInstance>,
	public IOleObjectImpl<CPluginInstance>,
	public IOleInPlaceActiveObjectImpl<CPluginInstance>,
	public IViewObjectExImpl<CPluginInstance>,
	public IOleInPlaceObjectWindowlessImpl<CPluginInstance>,
	public ISupportErrorInfo,
	public IPersistStorageImpl<CPluginInstance>,
	public ISpecifyPropertyPagesImpl<CPluginInstance>,
	public IQuickActivateImpl<CPluginInstance>,
#ifndef _WIN32_WCE
	public IDataObjectImpl<CPluginInstance>,
#endif
	public IProvideClassInfo2Impl<&CLSID_pluginInstance, NULL, &LIBID_webrtc4ieLib>,
#ifdef _WIN32_WCE // IObjectSafety is required on Windows CE for the control to be loaded correctly
	public IObjectSafetyImpl<CPluginInstance, INTERFACESAFE_FOR_UNTRUSTED_CALLER>,
#endif
	public CComCoClass<CPluginInstance, &CLSID_pluginInstance>,
	public CComControl<CPluginInstance>,
	public IObjectSafetyImpl<CPluginInstance, INTERFACESAFE_FOR_UNTRUSTED_CALLER | INTERFACESAFE_FOR_UNTRUSTED_DATA>
{
public:


	CPluginInstance() : _PluginInstance()
	{
		m_bWindowOnly = TRUE;
	}

DECLARE_OLEMISC_STATUS(OLEMISC_RECOMPOSEONRESIZE |
	OLEMISC_CANTLINKINSIDE |
	OLEMISC_INSIDEOUT |
	OLEMISC_ACTIVATEWHENVISIBLE |
	OLEMISC_SETCLIENTSITEFIRST
)

DECLARE_REGISTRY_RESOURCEID(IDR_PLUGININSTANCE)


BEGIN_COM_MAP(CPluginInstance)
	COM_INTERFACE_ENTRY(IPluginInstance)
	COM_INTERFACE_ENTRY(IDispatch)
	COM_INTERFACE_ENTRY(IViewObjectEx)
	COM_INTERFACE_ENTRY(IViewObject2)
	COM_INTERFACE_ENTRY(IViewObject)
	COM_INTERFACE_ENTRY(IOleInPlaceObjectWindowless)
	COM_INTERFACE_ENTRY(IOleInPlaceObject)
	COM_INTERFACE_ENTRY2(IOleWindow, IOleInPlaceObjectWindowless)
	COM_INTERFACE_ENTRY(IOleInPlaceActiveObject)
	COM_INTERFACE_ENTRY(IOleControl)
	COM_INTERFACE_ENTRY(IOleObject)
	COM_INTERFACE_ENTRY(IPersistStreamInit)
	COM_INTERFACE_ENTRY2(IPersist, IPersistStreamInit)
	COM_INTERFACE_ENTRY(ISupportErrorInfo)
	COM_INTERFACE_ENTRY(ISpecifyPropertyPages)
	COM_INTERFACE_ENTRY(IQuickActivate)
	COM_INTERFACE_ENTRY(IPersistStorage)
#ifndef _WIN32_WCE
	COM_INTERFACE_ENTRY(IDataObject)
#endif
	COM_INTERFACE_ENTRY(IProvideClassInfo)
	COM_INTERFACE_ENTRY(IProvideClassInfo2)
#ifdef _WIN32_WCE // IObjectSafety is required on Windows CE for the control to be loaded correctly
	COM_INTERFACE_ENTRY_IID(IID_IObjectSafety, IObjectSafety)
#endif
	COM_INTERFACE_ENTRY(IObjectSafety)
END_COM_MAP()

BEGIN_PROP_MAP(CPluginInstance)
	PROP_DATA_ENTRY("_cx", m_sizeExtent.cx, VT_UI4)
	PROP_DATA_ENTRY("_cy", m_sizeExtent.cy, VT_UI4)
	PROP_ENTRY_TYPE("AutoSize", DISPID_AUTOSIZE, CLSID_NULL, VT_BOOL)
	PROP_ENTRY_TYPE("BorderWidth", DISPID_BORDERWIDTH, CLSID_NULL, VT_I4)
	// Example entries
	// PROP_ENTRY_TYPE("Property Name", dispid, clsid, vtType)
	// PROP_PAGE(CLSID_StockColorPage)
END_PROP_MAP()


BEGIN_MSG_MAP(CPluginInstance)
	CHAIN_MSG_MAP(CComControl<CPluginInstance>)
	DEFAULT_REFLECTION_HANDLER()
END_MSG_MAP()
// Handler prototypes:
//  LRESULT MessageHandler(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
//  LRESULT CommandHandler(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
//  LRESULT NotifyHandler(int idCtrl, LPNMHDR pnmh, BOOL& bHandled);

// ISupportsErrorInfo
	STDMETHOD(InterfaceSupportsErrorInfo)(REFIID riid)
	{
		static const IID* arr[] =
		{
			&IID_IPluginInstance,
		};

		for (int i=0; i<sizeof(arr)/sizeof(arr[0]); i++)
		{
			if (InlineIsEqualGUID(*arr[i], riid))
				return S_OK;
		}
		return S_FALSE;
	}

// IViewObjectEx
	DECLARE_VIEW_STATUS(VIEWSTATUS_SOLIDBKGND | VIEWSTATUS_OPAQUE)

// IPluginInstance
public:
		HRESULT OnDraw(ATL_DRAWINFO& di)
		{
		RECT& rc = *(RECT*)di.prcBounds;
#if METROPOLIS
		FillRect(di.hdcDraw, &rc, (HBRUSH)GetStockObject(BLACK_BRUSH));
#else
		// Set Clip region to the rectangle specified by di.prcBounds
		HRGN hRgnOld = NULL;
		if (GetClipRgn(di.hdcDraw, hRgnOld) != 1)
			hRgnOld = NULL;
		bool bSelectOldRgn = false;

		HRGN hRgnNew = CreateRectRgn(rc.left, rc.top, rc.right, rc.bottom);

		if (hRgnNew != NULL)
		{
			bSelectOldRgn = (SelectClipRgn(di.hdcDraw, hRgnNew) != ERROR);
		}

		Rectangle(di.hdcDraw, rc.left, rc.top, rc.right, rc.bottom);
		SetTextAlign(di.hdcDraw, TA_CENTER|TA_BASELINE);
		LPCTSTR pszText = _T("ATL 8.0 : Doubango Telecom PluginInstance");
#ifndef _WIN32_WCE
		TextOut(di.hdcDraw,
			(rc.left + rc.right) / 2,
			(rc.top + rc.bottom) / 2,
			pszText,
			lstrlen(pszText));
#else
		ExtTextOut(di.hdcDraw,
			(rc.left + rc.right) / 2,
			(rc.top + rc.bottom) / 2,
			ETO_OPAQUE,
			NULL,
			pszText,
			ATL::lstrlen(pszText),
			NULL);
#endif

		if (bSelectOldRgn)
			SelectClipRgn(di.hdcDraw, hRgnOld);
#endif

		return S_OK;
	}


	DECLARE_PROTECT_FINAL_CONSTRUCT()

	HRESULT FinalConstruct()
	{
		return S_OK;
	}

	void FinalRelease()
	{
	}

	STDMETHOD(get_version)(BSTR* pVal);
	STDMETHOD(get_supportsPeerConnection)(VARIANT_BOOL* pVal);
	STDMETHOD(get_supportsSessionDescription)(VARIANT_BOOL* pVal);
	STDMETHOD(get_supportsNetTransport)(VARIANT_BOOL* pVal);
	STDMETHOD(get_windowHandle)(LONGLONG* pVal);

	STDMETHOD(get_fps)(LONG* pVal);
	STDMETHOD(put_fps)(LONG pVal);
	STDMETHOD(get_maxVideoSize)(BSTR* pVal);
	STDMETHOD(put_maxVideoSize)(BSTR pVal);
	STDMETHOD(get_maxBandwidthUp)(LONG* pVal);
	STDMETHOD(put_maxBandwidthUp)(LONG pVal);
	STDMETHOD(get_maxBandwidthDown)(LONG* pVal);
	STDMETHOD(put_maxBandwidthDown)(LONG pVal);
	STDMETHOD(get_zeroArtifacts)(VARIANT_BOOL* pVal);
	STDMETHOD(put_zeroArtifacts)(VARIANT_BOOL newVal);
	
	STDMETHOD(createPeerConnection)(IDispatch** ppPeerConnection);
	STDMETHOD(createSessionDescription)(IDispatch** ppSessionDescription);
	STDMETHOD(createNetTransport)(IDispatch** ppNetTransport);

	STDMETHOD(runningApps)(BSTR* pApps);
};

OBJECT_ENTRY_AUTO(__uuidof(pluginInstance), CPluginInstance)
