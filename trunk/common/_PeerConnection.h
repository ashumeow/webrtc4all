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
#ifndef _WEBRTC4ALL__PEERCONNECTION_H_
#define _WEBRTC4ALL__PEERCONNECTION_H_

#include "_Config.h"

typedef enum IceOptions_e
{
	IceOptions_All,
	IceOptions_NoRelay,
	IceOptions_OnlyRelay
}
IceOptions_t;

typedef enum ReadyState_e
{
	ReadyStateNew = 0, // initial state
	ReadyStateOpening = 1, // local or remote desc set
	ReadyStateActive = 2,  // local and remote desc set
	ReadyStateClosed = 3
}
ReadyState_t;
	
typedef enum IceState_e
{
	IceStateGathering = 0x100,
    IceStateWaiting = 0x200,
    IceStateChecking = 0x300,
    IceStateConnected = 0x400,
    IceStateCompleted = 0x500,
    IceStateFailed = 0x600,
    IceStateClosed = 0x700
}
IceState_t;

typedef enum SdpAction_e
{
	SdpActionOffer = 0x100,
	SdpActionProvisionalAnswer = 0x200,
	SdpActionAnswer = 0x300
}
SdpAction_t;

class _SessionDescription;

class PeerConnectionEvent
{
public:
	PeerConnectionEvent(const char* media, const char* candidate, bool moreToFollow):
		mMedia(tsk_strdup(media)),
		mCandidate(tsk_strdup(candidate)),
		mMoreToFollow(moreToFollow)
		{ }

	~PeerConnectionEvent()
	{
		TSK_FREE(mMedia);
		TSK_FREE(mCandidate);
	}

	const char* GetMedia()const{ return mMedia; }
	const char* GetCandidate()const{ return mCandidate; }
	bool GetMoreToFollow()const{ return mMoreToFollow; }

private:
	char* mMedia;
	char* mCandidate;
	bool mMoreToFollow;

};


class _PeerConnection {
public:
  _PeerConnection(BrowserType_t browserType);
  virtual ~_PeerConnection();

 protected:
	virtual bool Close();
	virtual bool CreateOffer(bool has_audio, bool has_video, char** sdp, int* sdp_len);
	virtual bool CreateAnswer(bool has_audio, bool has_video, char** sdp, int* sdp_len);
	virtual bool StartIce(int IceOptions);
	virtual bool SetLocalDescription(int action, const _SessionDescription* sdpObj);
	virtual bool SetRemoteDescription(int action, const _SessionDescription* sdpObj);

	virtual bool _Init(const char* conf, int conf_len);
	virtual bool _StartDebug(void);
	virtual bool _StopDebug(void);

	virtual bool SetDisplays(LONGLONG local, LONGLONG remote);
	virtual bool SetDisplayLocal(LONGLONG local);
	virtual bool SetDisplayRemote(LONGLONG remote);


protected:
	virtual bool CreateSessionMgr(enum tmedia_type_e eMediaType, bool offerer);
	virtual bool CreateLo(bool has_audio, bool has_video, char** sdpStr, int* sdp_len, bool offerer);
	virtual bool SerializeSdp(const struct tsdp_message_s* sdp, char** sdpStr, int* sdp_len);
	virtual bool IceCreateCtx(enum tmedia_type_e eMediaType);
	virtual bool IceSetTimeout(int32_t timeout);
	virtual bool IceGotLocalCandidates();
	virtual bool IceProcessRo(const struct tsdp_message_s* sdp_ro, bool isOffer);
	virtual bool IceIsConnected();
	static int IceCallback(const struct tnet_ice_event_s *e);

public:
	virtual void StartMedia();
	virtual void IceCallbackFire(const PeerConnectionEvent* e) = 0;
	virtual LONGLONG GetWindowHandle() = 0;

protected:
	enum tmedia_type_e mMediaType;
	struct tmedia_session_mgr_s* mSessionMgr;

	struct tsdp_message_s* mSdpLocal;
	struct tsdp_message_s* mSdpRemote;
	
	struct tnet_ice_ctx_s *mIceCtxAudio;
	struct tnet_ice_ctx_s *mIceCtxVideo;

	enum ReadyState_e mReadyState;
	enum IceState_e mIceState;

	LONGLONG mRemoteVideo;
	LONGLONG mLocalVideo;

	bool mFullScreen;

	BrowserType_t mBrowserType;

	CRITICAL_SECTION mCSIceCallback;
};

#endif /* _WEBRTC4ALL__PEERCONNECTION_H_ */
