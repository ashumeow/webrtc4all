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
#include "SessionDescriptionIE.h"

#include <comutil.h>
#include <stdio.h>

// CSessionDescription

STDMETHODIMP CSessionDescription::InterfaceSupportsErrorInfo(REFIID riid)
{
	static const IID* arr[] = 
	{
		&IID_ISessionDescription
	};

	for (int i=0; i < sizeof(arr) / sizeof(arr[0]); i++)
	{
		if (InlineIsEqualGUID(*arr[i],riid))
			return S_OK;
	}
	return S_FALSE;
}

CSessionDescription::CSessionDescription()
:_SessionDescription()
{
}

CSessionDescription::~CSessionDescription()
{
	
}

STDMETHODIMP CSessionDescription::toSdp(BSTR* sdp)
{
	if(!GetSdp()){
		TSK_DEBUG_ERROR("Not initialized");
		return E_FAIL;
	}

	char* sdp_str = tsdp_message_tostring(GetSdp());
	if(!sdp_str){
		TSK_DEBUG_ERROR("Cannot serialize local offer");
		return E_FAIL;
	}

	 _bstr_t bstr(sdp_str);
	 *sdp = bstr.GetBSTR();

	 TSK_FREE(sdp_str);

	return S_OK;
}


STDMETHODIMP CSessionDescription::Init(BSTR sdp)
{
	if(!sdp){
		TSK_DEBUG_ERROR("Invalid argument");
		return E_INVALIDARG;
	}

	char* sdpStr = _com_util::ConvertBSTRToString(sdp);
	bool ret = _SessionDescription::Init(sdpStr, tsk_strlen(sdpStr));
	TSK_FREE(sdpStr);

	return (ret ? S_OK : E_FAIL);
}

STDMETHODIMP CSessionDescription::addCandidate(BSTR media, BSTR candidate)
{
	if(!candidate || !media){
		TSK_DEBUG_ERROR("Invalid argument");
		return E_INVALIDARG;
	}
	

	char* candidateStr = _com_util::ConvertBSTRToString(candidate);
	char* mediaStr = _com_util::ConvertBSTRToString(media);

	bool ret = _SessionDescription::AddCandidate(mediaStr, candidateStr);
	
	TSK_FREE(candidateStr);
	TSK_FREE(mediaStr);

	return (ret ? S_OK : E_FAIL);
}
