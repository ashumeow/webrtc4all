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
#include "NetTransportIE.h"
#include "UtilsIE.h"

#include <comutil.h>
#include <stdio.h>

STDMETHODIMP CNetTransport::InterfaceSupportsErrorInfo(REFIID riid)
{
	static const IID* arr[] = 
	{
		&IID_INetTransport
	};

	for (int i=0; i < sizeof(arr) / sizeof(arr[0]); i++)
	{
		if (InlineIsEqualGUID(*arr[i],riid))
			return S_OK;
	}
	return S_FALSE;
}

CNetTransport::CNetTransport():
_NetTransport(),
mLooperHandle(NULL),
mLooperProc(NULL)
{
}

CNetTransport::~CNetTransport()
{
	if(mLooperHandle && mLooperProc){
		SetWindowLongPtr(mLooperHandle, GWL_WNDPROC, (LONG)mLooperProc);
	}
}

STDMETHODIMP CNetTransport::SetDomain(BSTR domain)
{
	if(!domain){
		TSK_DEBUG_ERROR("Invalid parameter");
		return E_INVALIDARG;
	}

	char* domainStr = _com_util::ConvertBSTRToString(domain);
	bool ret = _NetTransport::_SetDomain(domainStr);
	TSK_FREE(domainStr);

	return (ret ? S_OK : E_FAIL);
}

STDMETHODIMP CNetTransport::Start(LONGLONG looper)
{
	if(!looper){
		TSK_DEBUG_WARN("Starting without looper");
	}
	if(mLooperHandle && mLooperProc){
		SetWindowLongPtr(mLooperHandle, GWL_WNDPROC, (LONG)mLooperProc);
		mLooperProc = NULL;
	}
	if((mLooperHandle = (HWND)looper)){
		mLooperProc = (WNDPROC) SetWindowLongPtr(mLooperHandle, GWL_WNDPROC, (LONG)_Utils::WndProc);
		if(!mLooperProc){
			TSK_DEBUG_ERROR("SetWindowLongPtr() failed with errcode=%d", GetLastError());
		}
	}

	return _NetTransport::_Start(_NetTransport::DgramCb) ? S_OK : E_FAIL;
}

STDMETHODIMP CNetTransport::SendTo(BSTR msg, BSTR addr, USHORT port)
{
	if(!msg || !addr || !port){
		TSK_DEBUG_ERROR("Invalid parameter");
		return E_INVALIDARG;
	}

	char* addrStr = _com_util::ConvertBSTRToString(addr);
	char* msgStr = _com_util::ConvertBSTRToString(msg);
	bool ret = _NetTransport::_SendTo(msgStr, tsk_strlen(msgStr), addrStr, (int) port);	
	TSK_FREE(addrStr);
	TSK_FREE(msgStr);

	return (ret ? S_OK : E_FAIL);
}

STDMETHODIMP CNetTransport::get_localIP(BSTR* pVal)
{
	if(m_pTransport && m_pTransport->master){
		_bstr_t bstr(m_pTransport->master->ip);
		*pVal = bstr.GetBSTR();
	}
	else *pVal = NULL;
	return S_OK;
}

STDMETHODIMP CNetTransport::get_localPort(USHORT* pVal)
{
	if(m_pTransport && m_pTransport->master) *pVal =  m_pTransport->master->port;
	else *pVal = 0;
	return S_OK;
}

STDMETHODIMP CNetTransport::get_defaultDestAddr(BSTR* pVal)
{
	_bstr_t bstr(m_DefaultDestAddr);
	*pVal = bstr.GetBSTR();
	return S_OK;
}

STDMETHODIMP CNetTransport::get_defaultDestPort(USHORT* pVal)
{
	*pVal =  m_DefaultDestPort;
	return S_OK;
}

STDMETHODIMP CNetTransport::get_version(BSTR* pVal)
{
	*pVal = _bstr_t(THIS_VERSION);
	return S_OK;
}

STDMETHODIMP CNetTransport::Stop(void)
{
	if(m_pTransport && TSK_RUNNABLE(m_pTransport)->started){
		int iRet;
		if((iRet = tnet_transport_shutdown(m_pTransport)) != 0){
			TSK_DEBUG_ERROR("Failed to stop transport [%d]", iRet);
			return E_FAIL;
		}
	}

	return S_OK;
}

STDMETHODIMP CNetTransport::StartDebug()
{
	_Utils::StartDebug();
	return S_OK;
}

STDMETHODIMP CNetTransport::StopDebug()
{
	_Utils::StopDebug();
	return S_OK;
}

LONGLONG CNetTransport::GetWindowHandle()
{
	return (LONGLONG)mLooperHandle;
}

void CNetTransport::DgramCbFire(const NetTransportEvent* e)
{
	USHORT eType = e->GetType();
	_bstr_t oData = Utils::ToBSTR((const char*)e->GetData(), e->GetDataLen());
	Fire_OnEvent(eType, oData);
	::SysFreeString(oData);
}
