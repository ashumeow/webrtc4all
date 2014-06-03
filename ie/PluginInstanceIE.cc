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
#include "pluginInstanceIE.h"
#include "PeerConnectionIE.h"
#include "NetTransportIE.h"
#include "SessionDescriptionIE.h"
#include "UtilsIE.h"


STDMETHODIMP CPluginInstance::get_version(BSTR* pVal)
{
	*pVal = Utils::SysAllocStringBytes(_PluginInstance::GetVersion());
	return S_OK;
}

STDMETHODIMP CPluginInstance::get_supportsPeerConnection(VARIANT_BOOL* pVal)
{
	*pVal = _PluginInstance::GetSupportsPeerConnection() ? VARIANT_TRUE : VARIANT_FALSE;
	return S_OK;
}

STDMETHODIMP CPluginInstance::get_supportsSessionDescription(VARIANT_BOOL* pVal)
{
	*pVal = _PluginInstance::GetSupportsSessionDescription() ? VARIANT_TRUE : VARIANT_FALSE;
	return S_OK;
}

STDMETHODIMP CPluginInstance::get_supportsNetTransport(VARIANT_BOOL* pVal)
{
	*pVal = _PluginInstance::GetSupportsNetTransport() ? VARIANT_TRUE : VARIANT_FALSE;
	return S_OK;
}

STDMETHODIMP CPluginInstance::get_windowHandle(LONGLONG* pVal)
{
	*pVal = reinterpret_cast<LONGLONG>(m_hWnd);	
	return S_OK;
}

STDMETHODIMP CPluginInstance::get_fps(LONG* pVal)
{
	*pVal = _PluginInstance::GetFps();
	return S_OK;
}

STDMETHODIMP CPluginInstance::put_fps(LONG pVal)
{
	return _PluginInstance::SetFps(pVal) ? S_OK : E_FAIL;
}

STDMETHODIMP CPluginInstance::get_maxVideoSize(BSTR* pVal)
{
	*pVal = Utils::SysAllocStringBytes(_PluginInstance::GetMaxVideoSize());
	return S_OK;
}

STDMETHODIMP CPluginInstance::put_maxVideoSize(BSTR pVal)
{
	char* _maxSize = _com_util::ConvertBSTRToString(pVal);
	HRESULT hr = _PluginInstance::SetMaxVideoSize(_maxSize) ? S_OK : E_FAIL;
	TSK_FREE(_maxSize);
	return hr;
}

STDMETHODIMP CPluginInstance::get_maxBandwidthUp(LONG* pVal)
{
	*pVal = _PluginInstance::GetMaxBandwidthUp();
	return S_OK;
}

STDMETHODIMP CPluginInstance::put_maxBandwidthUp(LONG pVal)
{
	return _PluginInstance::SetMaxBandwidthUp(pVal) ? S_OK : E_FAIL;
}

STDMETHODIMP CPluginInstance::get_maxBandwidthDown(LONG* pVal)
{
	*pVal = _PluginInstance::GetMaxBandwidthDown();
	return S_OK;
}

STDMETHODIMP CPluginInstance::put_maxBandwidthDown(LONG pVal)
{
	return _PluginInstance::SetMaxBandwidthDown(pVal) ? S_OK : E_FAIL;
}

STDMETHODIMP CPluginInstance::createPeerConnection(IDispatch** ppPeerConnection)
{
	CComObject<CPeerConnection>* pPeerConnection;
    HRESULT hr = CComObject<CPeerConnection>::CreateInstance(&pPeerConnection);
    if (FAILED(hr)) {
        return hr;
    }
    pPeerConnection->AddRef();
    *ppPeerConnection = pPeerConnection;
    return S_OK;
}

STDMETHODIMP CPluginInstance::createSessionDescription(IDispatch** ppSessionDescription)
{
	CComObject<CSessionDescription>* pSessionDescription;
    HRESULT hr = CComObject<CSessionDescription>::CreateInstance(&pSessionDescription);
    if (FAILED(hr)) {
        return hr;
    }
    pSessionDescription->AddRef();
    *ppSessionDescription = pSessionDescription;
    return S_OK;
}

STDMETHODIMP CPluginInstance::createNetTransport(IDispatch** ppNetTransport)
{
	CComObject<CNetTransport>* pNetTransport;
    HRESULT hr = CComObject<CNetTransport>::CreateInstance(&pNetTransport);
    if (FAILED(hr)) {
        return hr;
    }
    pNetTransport->AddRef();
    *ppNetTransport = pNetTransport;
    return S_OK;
}

STDMETHODIMP CPluginInstance::runningApps(BSTR* pApps)
{
	_ActiveApps* activeApps = _Utils::GetActiveApps();
	if (!activeApps || !activeApps->GetDataPtr() || !activeApps->GetDataSize()) {
		if (activeApps) {
			delete activeApps;
		}
		return E_FAIL;
	}
	
	*pApps = Utils::SysAllocStringBytes((LPCSTR)activeApps->GetDataPtr());
	
	delete activeApps;
	
	return S_OK;
}
