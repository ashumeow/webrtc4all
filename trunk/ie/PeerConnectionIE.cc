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
#include "PeerConnectionIE.h"

#include "../common/_SessionDescription.h"

#include <comutil.h>
#include <stdio.h>

STDMETHODIMP CPeerConnection::InterfaceSupportsErrorInfo(REFIID riid)
{
	static const IID* arr[] = 
	{
		&IID_IPeerConnection
	};

	for (int i=0; i < sizeof(arr) / sizeof(arr[0]); i++)
	{
		if (InlineIsEqualGUID(*arr[i],riid))
			return S_OK;
	}
	return S_FALSE;
}

CPeerConnection::CPeerConnection():
_PeerConnection(),
mLooperHandle(NULL),
mLooperProc(NULL)
{
}

CPeerConnection::~CPeerConnection()
{
}

STDMETHODIMP CPeerConnection::close(void)
{
	bool ret = _PeerConnection::Close();
	
	if(mLooperHandle && mLooperProc){
		SetWindowLongPtr(mLooperHandle, GWL_WNDPROC, (LONG)mLooperProc);
		mLooperProc = NULL;
	}

	return (ret ? S_OK : E_FAIL);
}

STDMETHODIMP CPeerConnection::createOffer(VARIANT_BOOL has_audio, VARIANT_BOOL has_video, BSTR* offer)
{
	char *_offer = NULL;
	int _offer_len;
	bool ret = _PeerConnection::CreateLo((has_audio == VARIANT_TRUE), (has_video == VARIANT_TRUE), &_offer, &_offer_len, true);
	if(_offer){
		bstr_t bstr(_offer);
		*offer = bstr.GetBSTR();
	}
	TSK_FREE(_offer);
	return ret ? S_OK : E_FAIL;
}

STDMETHODIMP CPeerConnection::createAnswer(VARIANT_BOOL has_audio, VARIANT_BOOL has_video, BSTR* answer)
{
	char *_answer = NULL;
	int _answer_len;
	bool ret = _PeerConnection::CreateLo((has_audio == VARIANT_TRUE), (has_video == VARIANT_TRUE), &_answer, &_answer_len, false);
	if(_answer){
		bstr_t bstr(_answer);
		*answer = bstr.GetBSTR();
	}
	TSK_FREE(_answer);
	return ret ? S_OK : E_FAIL;
}


STDMETHODIMP CPeerConnection::startIce(SHORT IceOptions, LONGLONG looper)
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

	return _PeerConnection::StartIce(IceOptions) ? S_OK : E_FAIL;
}

STDMETHODIMP CPeerConnection::setLocalDescription(USHORT action, BSTR desc)
{
	char* sdpStr = _com_util::ConvertBSTRToString(desc);
	_SessionDescription sdpObj;
	bool ret = sdpObj.Init(sdpStr, tsk_strlen(sdpStr));
	ret &= _PeerConnection::SetLocalDescription(action, &sdpObj);
	TSK_FREE(sdpStr);
	return (ret ? S_OK : E_FAIL);
}

STDMETHODIMP CPeerConnection::setRemoteDescription(USHORT action, BSTR desc)
{
	char* sdpStr = _com_util::ConvertBSTRToString(desc);
	_SessionDescription sdpObj;
	bool ret = sdpObj.Init(sdpStr, tsk_strlen(sdpStr));
	ret &= _PeerConnection::SetRemoteDescription(action, &sdpObj);
	TSK_FREE(sdpStr);
	return (ret ? S_OK : E_FAIL);
}

STDMETHODIMP CPeerConnection::get_localDescription(BSTR* pVal)
{
	char* sdpStr = NULL;
	int sdpLen = 0;
	bool ret = _PeerConnection::SerializeSdp(mSdpLocal, &sdpStr, &sdpLen);
	if(ret){
		bstr_t bstr(sdpStr);
		*pVal = bstr.GetBSTR();
	}
	TSK_FREE(sdpStr);
	return (ret ? S_OK : E_FAIL);
}

STDMETHODIMP CPeerConnection::get_remoteDescription(BSTR* pVal)
{
	char* sdpStr = NULL;
	int sdpLen = 0;
	bool ret = _PeerConnection::SerializeSdp(mSdpRemote, &sdpStr, &sdpLen);
	if(ret){
		bstr_t bstr(sdpStr);
		*pVal = bstr.GetBSTR();
	}
	TSK_FREE(sdpStr);
	return (ret ? S_OK : E_FAIL);
}

STDMETHODIMP CPeerConnection::get_readyState(USHORT* pVal)
{
	*pVal = (USHORT)mReadyState;
	return S_OK;
}

STDMETHODIMP CPeerConnection::get_iceState(USHORT* pVal)
{
	*pVal = (USHORT)mIceState;
	return S_OK;
}

STDMETHODIMP CPeerConnection::get_remoteVideo(LONGLONG* pVal)
{
	*pVal = mRemoteVideo;
	return S_OK;
}

STDMETHODIMP CPeerConnection::put_remoteVideo(LONGLONG newVal)
{
	mRemoteVideo = newVal;
	return _PeerConnection::SetDisplays(mLocalVideo, mRemoteVideo) ? S_OK : E_FAIL;
}

STDMETHODIMP CPeerConnection::get_localVideo(LONGLONG* pVal)
{
	*pVal = mLocalVideo;
	return S_OK;
}

STDMETHODIMP CPeerConnection::put_localVideo(LONGLONG newVal)
{
	mLocalVideo = newVal;
	return _PeerConnection::SetDisplays(mLocalVideo, mRemoteVideo) ? S_OK : E_FAIL;
}

STDMETHODIMP CPeerConnection::get_version(BSTR* pVal)
{
	*pVal = _bstr_t(THIS_VERSION);
	return S_OK;
}

STDMETHODIMP CPeerConnection::get_fullScreen(VARIANT_BOOL* pVal)
{
	*pVal = mFullScreen;
	return S_OK;
}

STDMETHODIMP CPeerConnection::put_fullScreen(VARIANT_BOOL newVal)
{
	mFullScreen = (newVal == VARIANT_TRUE);
	if(mSessionMgr){
		int32_t fs = mFullScreen ? 1 : 0;
		tmedia_session_mgr_set(mSessionMgr,
			TMEDIA_SESSION_CONSUMER_SET_INT32(tmedia_video, "fullscreen", fs),
			TMEDIA_SESSION_SET_NULL());
	}
	
	return S_OK;
}

STDMETHODIMP CPeerConnection::Init(BSTR configuration)
{
	if(configuration){ // e.g. STUN 'stun.l.google.com:19302'
		char* confStr = _com_util::ConvertBSTRToString(configuration);
		bool ret = _PeerConnection::_Init(confStr, tsk_strlen(confStr));
		TSK_FREE(confStr);
		return (ret ? S_OK : E_FAIL);
	}
	return S_OK;
}

STDMETHODIMP CPeerConnection::StartDebug(void)
{
	return (_PeerConnection::_StartDebug() ? S_OK : E_FAIL);
}

STDMETHODIMP CPeerConnection::StopDebug(void)
{
	return (_PeerConnection::_StopDebug() ? S_OK : E_FAIL);
}

void CPeerConnection::IceCallbackFire(const PeerConnectionEvent* e)
{
	bstr_t media(e->GetMedia());
	bstr_t candidate(e->GetCandidate());
	Fire_IceCallback(media, candidate, e->GetMoreToFollow());
}

LONGLONG CPeerConnection::GetWindowHandle()
{
	return (LONGLONG)mLooperHandle;
}