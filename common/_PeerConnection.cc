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
#include "_PeerConnection.h"
#include "_Utils.h"
#include "../common/_SessionDescription.h"

#include <sys/stat.h> /* stat() */

#if HAVE_LIBXML2
#include <libxml/tree.h>
#include <libxml/parser.h>
#include <libxml/xpath.h>
#include <libxml/xpathInternals.h>
#endif

static BOOL g_bAlwaysCreateOnCurrentThread = TRUE; // because "ThreadingModel 'Apartment'"
static BOOL g_bAVPF = TRUE;

_PeerConnection::_PeerConnection(BrowserType_t browserType): 
	mBrowserType(browserType),
	mRemoteVideo(0),
	mLocalVideo(0),
	mReadyState(ReadyStateNew),
	mIceState(IceStateClosed),
	mSessionMgr(NULL),
	mMediaType(tmedia_none),
	mSdpLocal(NULL),
	mSdpRemote(NULL),
	mIceCtxAudio(NULL),
	mIceCtxVideo(NULL),
	mRemoteHaveIce(false),
	mStartDelayedUntilIceDone(false),
	mFullScreen(false)
{
	_Utils::Initialize();
	InitializeCriticalSection(&mCSIceCallback);
}

_PeerConnection::~_PeerConnection()
{
	Close();
	
	SetDisplays(NULL, NULL);
	
	TSK_OBJECT_SAFE_FREE(mSdpLocal);
	TSK_OBJECT_SAFE_FREE(mSdpRemote);
	
	DeleteCriticalSection(&mCSIceCallback);
}

bool _PeerConnection::Close()
{
	TSK_OBJECT_SAFE_FREE(mSessionMgr);
	TSK_OBJECT_SAFE_FREE(mIceCtxAudio);
	TSK_OBJECT_SAFE_FREE(mIceCtxVideo);

	mReadyState = ReadyStateClosed;
	mIceState = IceStateClosed;

	return true;
}

bool _PeerConnection::CreateOffer(bool has_audio, bool has_video, char** sdp, int* sdp_len)
{
	return CreateLo(has_audio, has_video, sdp, sdp_len, true);
}

bool _PeerConnection::CreateAnswer(bool has_audio, bool has_video, char** sdp, int* sdp_len)
{
	return CreateLo(has_audio, has_video, sdp, sdp_len, false);
}

bool _PeerConnection::StartIce(int IceOptions)
{
	IceOptions_t options = (IceOptions_t)IceOptions;

	(options);

	if(!mSdpLocal){
		TSK_DEBUG_ERROR("No local offer yet");
		return false;
	}

	if(!mIceCtxAudio && !mIceCtxVideo) {
		TSK_DEBUG_INFO("ICE is not enabled...");
		mIceState = IceStateCompleted;
		return SignalNoMoreIceCandidateToFollow(); // say to the browser that it should not wait for any ICE candidate callback
	}

	int iRet;

	if((mMediaType & tmedia_audio)){
		if(mIceCtxAudio && (iRet = tnet_ice_ctx_start(mIceCtxAudio)) != 0){
			TSK_DEBUG_WARN("tnet_ice_ctx_start() failed with error code = %d", iRet);
			return false;
		}
	}
	if((mMediaType & tmedia_video)){
		if(mIceCtxVideo && (iRet = tnet_ice_ctx_start(mIceCtxVideo)) != 0){
			TSK_DEBUG_WARN("tnet_ice_ctx_start() failed with error code = %d", iRet);
			return false;
		}
	}

	return true;
}

bool _PeerConnection::StartMedia()
{
	if(mSessionMgr && mSdpLocal && mSdpRemote){
		if(mRemoteHaveIce && !IceIsDone()) {
			TSK_DEBUG_INFO("Delaying media start until ICE process is done");
			mStartDelayedUntilIceDone = true;
			return true;
		}
		mStartDelayedUntilIceDone = false;
		
		// set callback functions now that all sessions are up
		// tmedia_session_mgr_set_onerror_cbfn(self->msession_mgr, self, tsip_dialog_invite_msession_onerror_cb);
		tmedia_session_mgr_set_rfc5168_cbfn(mSessionMgr, this, _PeerConnection::Rfc5168Callback);

		return (tmedia_session_mgr_start(mSessionMgr) == 0);
	}
	TSK_DEBUG_ERROR("Not ready");
	return false;
}

bool _PeerConnection::SetLocalDescription(int action, const _SessionDescription* sdpObj)
{
	(action);

	if(mReadyState == ReadyStateActive){
#if 0 // ICE never timeout: never setup a call without ICE
		IceSetTimeout(ICE_TIMEOUT);
#endif
	 }

	return true;
}

bool _PeerConnection::SetRemoteDescription(int action, const _SessionDescription* sdpObj)
{
	int iRet = 0;

	if(!sdpObj){
		TSK_DEBUG_ERROR("Invalid argument");
		iRet = -1;
		goto bail;
	}

	tmedia_ro_type_t ro_type = (action == SdpActionProvisionalAnswer) 
		? tmedia_ro_type_provisional
		: (action == SdpActionAnswer ? tmedia_ro_type_answer : tmedia_ro_type_offer);

	mRemoteHaveIce = IceIsEnabled(sdpObj->GetSdp());

	if(!mSessionMgr){
		if(!CreateSessionMgr(tmedia_type_from_sdp(sdpObj->GetSdp()), mRemoteHaveIce, false)){
			iRet = -1;
			goto bail;
		}
	}
	
	if((iRet = tmedia_session_mgr_set_ro(mSessionMgr, sdpObj->GetSdp(), ro_type)) != 0){
		TSK_DEBUG_ERROR("tmedia_session_mgr_set_ro() failed with error code = %d", iRet);
		goto bail;
	}

	switch(action){
		case SdpActionOffer:
			{
				if(mRemoteHaveIce && !(IceProcessRo(sdpObj->GetSdp(), true))){
					TSK_DEBUG_ERROR("Failed to process remote offer");
					goto bail;
				}
				break;
			}
		case SdpActionAnswer:
		case SdpActionProvisionalAnswer:
			{
				if(mRemoteHaveIce && !(IceProcessRo(sdpObj->GetSdp(), false))){
					TSK_DEBUG_ERROR("Failed to process remote offer");
					goto bail;
				}
				break;
			}
		default:
			{
				TSK_DEBUG_ERROR("Unknown action: %hu", action);
				iRet = -1;
				goto bail;
			}
	}

	TSK_OBJECT_SAFE_FREE(mSdpRemote);
	mSdpRemote = (tsdp_message_t*)tsk_object_ref((tsdp_message_t*)sdpObj->GetSdp());
	mReadyState = (mSdpLocal && mSdpRemote) ? ReadyStateActive : ReadyStateOpening;

	if(mRemoteHaveIce && mReadyState == ReadyStateActive){
		IceSetTimeout((tmedia_defaults_get_profile() == tmedia_profile_rtcweb) ? ICE_TIMEOUT_ENDLESS : ICE_TIMEOUT_VAL);
	 }
	
bail:
	return (iRet == 0);
}

bool _PeerConnection::_Init(const char* conf, int conf_len)
{
	return true;
}

bool _PeerConnection::_StartDebug(void)
{
	return _Utils::StartDebug();
}

bool _PeerConnection::_StopDebug(void)
{
	return _Utils::StopDebug();
}

bool _PeerConnection::SetDisplays(LONGLONG local, LONGLONG remote)
{
	mLocalVideo = (LONGLONG)local;
	mRemoteVideo = (LONGLONG)remote;

	if(mSessionMgr){
		tmedia_session_mgr_set(mSessionMgr,
			TMEDIA_SESSION_PRODUCER_SET_INT64(tmedia_video, "local-hwnd", mLocalVideo),
			TMEDIA_SESSION_CONSUMER_SET_INT64(tmedia_video, "remote-hwnd", mRemoteVideo),
				TMEDIA_SESSION_SET_NULL());
	}

	return true;
}

bool _PeerConnection::SetDisplayLocal(LONGLONG local)
{
	return SetDisplays(local, mRemoteVideo);
}

bool _PeerConnection::SetDisplayRemote(LONGLONG remote)
{
	return SetDisplays(mLocalVideo, remote);
}

bool _PeerConnection::ProcessContent(const char* req_name, const char* content_type, const void* content_ptr, int content_size)
{
	TSK_DEBUG_INFO("W4A::_PeerConnection::ProcessContent([%s], [%s], [%s])", req_name, content_type, content_ptr);
	if (tsk_strnullORempty(req_name) || tsk_strnullORempty(content_type) || !content_ptr || !content_size) {
		TSK_DEBUG_ERROR("W4A::_PeerConnection::ProcessContent: Invalid parameter")
		return false;
	}
	if (mSessionMgr && tsk_striequals("application/media_control+xml", content_type)){ /* rfc5168: XML Schema for Media Control */
		static uint32_t __ssrc_media_fake = 0;
		static tmedia_type_t __tmedia_type_video = tmedia_video; // TODO: add bfcpvideo?
		tsk_bool_t is_fir = tsk_false;
		uint64_t sessionId = 0;
#if HAVE_LIBXML2
		{
			xmlDoc *pDoc;
			xmlNode *pRootElement;
			xmlXPathContext *pPathCtx;
			xmlXPathObject *pPathObj;
			static const xmlChar* __xpath_expr_picture_fast_update = (const xmlChar*)"/media_control/vc_primitive/to_encoder/picture_fast_update";
			static const xmlChar* __xpath_expr_stream_id = (const xmlChar*)"/media_control/vc_primitive/stream_id";
			
			if (!(pDoc = xmlParseDoc((const xmlChar *)content_ptr))) {
				TSK_DEBUG_ERROR("Failed to parse XML content [%s]", content_ptr);
				return 0;
			}
			if (!(pRootElement = xmlDocGetRootElement(pDoc))) {
				TSK_DEBUG_ERROR("Failed to get root element from XML content [%s]", content_ptr);
				xmlFreeDoc(pDoc);
				return 0;
			}
			if (!(pPathCtx = xmlXPathNewContext(pDoc))) {
				TSK_DEBUG_ERROR("Failed to create path context from XML content [%s]", content_ptr);
				xmlFreeDoc(pDoc);
				return 0;
			}
			// picture_fast_update
			if (!(pPathObj = xmlXPathEvalExpression(__xpath_expr_picture_fast_update, pPathCtx))) {
				TSK_DEBUG_ERROR("Error: unable to evaluate xpath expression: %s", __xpath_expr_picture_fast_update);
				xmlXPathFreeContext(pPathCtx);
				xmlFreeDoc(pDoc);
				return 0;
			}
			is_fir = (pPathObj->type == XPATH_NODESET && pPathObj->nodesetval->nodeNr > 0);
			xmlXPathFreeObject(pPathObj);
			// stream_id
			if (!(pPathObj = xmlXPathEvalExpression(__xpath_expr_stream_id, pPathCtx))) {
				TSK_DEBUG_ERROR("Error: unable to evaluate xpath expression: %s", __xpath_expr_stream_id);
				xmlXPathFreeContext(pPathCtx);
				xmlFreeDoc(pDoc);
			}
			else if (pPathObj->type == XPATH_NODESET && pPathObj->nodesetval->nodeNr > 0 && pPathObj->nodesetval->nodeTab[0]->children && pPathObj->nodesetval->nodeTab[0]->children->content) {
				sessionId = tsk_atoi64((const char*)pPathObj->nodesetval->nodeTab[0]->children->content);
			}
			xmlXPathFreeObject(pPathObj);
			
			xmlXPathFreeContext(pPathCtx); 
			xmlFreeDoc(pDoc);
		}
#else
		is_fir = (tsk_strcontains(content_ptr, content_size, "to_encoder") && tsk_strcontains(content_ptr, content_size, "picture_fast_update"));
#endif
		TSK_DEBUG_INFO("Incoming Content(application/media_control+xml, picture_fast_update=%s)", is_fir ? "yes" : "no");
		if (is_fir) {
			TSK_DEBUG_INFO("Incoming SIP INFO(picture_fast_update)");
			if (sessionId) {
				tmedia_session_mgr_recv_rtcp_event_2(mSessionMgr, tmedia_rtcp_event_type_fir, sessionId);
			}
			else {
				tmedia_session_mgr_recv_rtcp_event(mSessionMgr, __tmedia_type_video, tmedia_rtcp_event_type_fir, __ssrc_media_fake);
			}
		}
	} //end-of-if("application/media_control+xml")
	return true;
}

bool _PeerConnection::CreateSessionMgr(tmedia_type_t eMediaType, bool iceEnabled, bool offerer)
{
	if(mSessionMgr){
		TSK_DEBUG_WARN("Session manager already defined");
		return true;
	}

	int iRet;
	tnet_ip_t bestsource;
	if((iRet = tnet_getbestsource("sipml5.org", 5060, tnet_socket_type_udp_ipv4, &bestsource))){
		TSK_DEBUG_ERROR("Failed to get best source [%d]", iRet);
		memcpy(bestsource, "0.0.0.0", 7);
	}
	if(!(mSessionMgr = tmedia_session_mgr_create(eMediaType, bestsource, USE_IPV6, offerer))){
		TSK_DEBUG_ERROR("Failed to create media session manager");
		return false;
	}

	mMediaType = eMediaType;
	if(iceEnabled && !IceCreateCtx(eMediaType)){
		TSK_DEBUG_ERROR("Failed to create ICE context");
		return false;
	}

	if(iceEnabled && (tmedia_session_mgr_set_ice_ctx(mSessionMgr, mIceCtxAudio, mIceCtxVideo) != 0)){
		TSK_DEBUG_ERROR("Failed to set ICE contexts");
		return false;
	}
	
	// DTLS certificates
	static char* __dtlsCertificate = tsk_null;
	static bool __dtlsCertificateExist = false;
	if(!__dtlsCertificate){
		tsk_sprintf(&__dtlsCertificate, "%s/pub.pem", _Utils::GetCurrentDirectoryPath());
		TSK_DEBUG_INFO("Path to DTLS-SRTP cert=%s", __dtlsCertificate);
	}
	// update for each call to be sure not added later
	struct stat _stat;
	__dtlsCertificateExist =((stat(__dtlsCertificate, &_stat) == 0 && _stat.st_size > 0));

	int32_t fs = (mFullScreen == true) ? 1 : 0;
	int32_t plugin_firefox  = (mBrowserType == BrowserType_Firefox || mBrowserType == BrowserType_Chrome) ? 1 : 0;
	static const int32_t plugin_webrtc4all = 1;
	tmedia_session_mgr_set(mSessionMgr,
			TMEDIA_SESSION_SET_INT32(mSessionMgr->type, "avpf-enabled", g_bAVPF), // Otherwise will be negociated using SDPCapNeg (RFC 5939)
			TMEDIA_SESSION_SET_STR(tmedia_audiovideo, "dtls-file-pbk",__dtlsCertificateExist ? __dtlsCertificate : tsk_null),
			TMEDIA_SESSION_PRODUCER_SET_INT64(tmedia_video, "local-hwnd", mLocalVideo),
			TMEDIA_SESSION_PRODUCER_SET_INT32(tmedia_video, "create-on-current-thead", g_bAlwaysCreateOnCurrentThread),
			TMEDIA_SESSION_PRODUCER_SET_INT32(tmedia_video, "plugin-firefox", plugin_firefox),
			TMEDIA_SESSION_CONSUMER_SET_INT64(tmedia_video, "remote-hwnd", mRemoteVideo),
			TMEDIA_SESSION_CONSUMER_SET_INT32(tmedia_video, "create-on-current-thead", g_bAlwaysCreateOnCurrentThread),
			TMEDIA_SESSION_CONSUMER_SET_INT32(tmedia_video, "fullscreen", fs),
			TMEDIA_SESSION_CONSUMER_SET_INT32(tmedia_video, "plugin-firefox", plugin_firefox),
			TMEDIA_SESSION_CONSUMER_SET_INT32(tmedia_video, "plugin-webrtc4all", plugin_webrtc4all),
			TMEDIA_SESSION_SET_NULL());

	return true;
}

bool _PeerConnection::CreateLo(bool has_audio, bool has_video, char** sdpStr, int* sdp_len, bool offerer)
{
	tmedia_type_t eMediaType = tmedia_none;
	if(has_audio) eMediaType = (tmedia_type_t)(eMediaType | tmedia_audio);
	if(has_video) eMediaType = (tmedia_type_t)(eMediaType | tmedia_video);

	*sdpStr = NULL;
	*sdp_len = 0;

	if(!mSessionMgr && !CreateSessionMgr(eMediaType, (mSdpRemote ? IceIsEnabled(mSdpRemote) : true), offerer)){
		return false;
	}
	
	const tsdp_message_t* sdp_lo = tmedia_session_mgr_get_lo(mSessionMgr);
	if(!sdp_lo){
		TSK_DEBUG_ERROR("Cannot get local offer");
		return false;
	}


	*sdpStr = tsdp_message_tostring(sdp_lo);
	if(!*sdpStr){
		TSK_DEBUG_ERROR("Cannot serialize local offer");
		return false;
	}
	*sdp_len = tsk_strlen(*sdpStr);

	 TSK_OBJECT_SAFE_FREE(mSdpLocal);
	 mSdpLocal = (tsdp_message_t*)tsk_object_ref((tsk_object_t*)sdp_lo);
	 mReadyState = (mSdpLocal && tmedia_session_mgr_get_ro(mSessionMgr)) ? ReadyStateActive : ReadyStateOpening;

	return true;
}

bool _PeerConnection::SerializeSdp(const tsdp_message_t* sdp, char** sdpStr, int* sdp_len)
{
	*sdpStr = NULL;
	*sdp_len = 0;
	if(sdp){
		*sdpStr = tsdp_message_tostring(sdp);
		if(!*sdpStr){
			TSK_DEBUG_ERROR("Cannot serialize local offer");
			return false;
		}
		*sdp_len = tsk_strlen(*sdpStr);
	}
	 
	return true;
}

// The JavaScript code subscribe to this callback to know that the SDP is ready to be sent
bool _PeerConnection::SignalNoMoreIceCandidateToFollow()
{
	HWND hWnd = reinterpret_cast<HWND>(GetWindowHandle());
	static const bool kMoreToFollowIsFalse = false;
	PeerConnectionEvent* oEvent = new PeerConnectionEvent(tsk_null, tsk_null, kMoreToFollowIsFalse);
	if(!PostMessageA(hWnd, WM_ICE_EVENT_CANDIDATE, reinterpret_cast<WPARAM>(this), reinterpret_cast<LPARAM>(oEvent))){
		TSK_DEBUG_ERROR("PostMessageA() failed");
		IceCallbackFire(oEvent);
		if(oEvent) delete oEvent, oEvent = NULL; // even will be destroyed by the listener if succeed
	}
	return true;
}

bool _PeerConnection::IceCreateCtx(tmedia_type_t _eMediaType)
{
	if(!mIceCtxAudio && (_eMediaType & tmedia_audio)){
		mIceCtxAudio = tnet_ice_ctx_create(USE_ICE_JINGLE, USE_IPV6, USE_ICE_RTCP, FALSE/*audio*/, &_PeerConnection::IceCallback, this);
		if(!mIceCtxAudio){
			TSK_DEBUG_ERROR("Failed to create ICE audio context");
			return false;
		}
		tnet_ice_ctx_set_stun(mIceCtxAudio, "stun.l.google.com", 19302, "Doubango", "stun-username", "stun-password"); //FIXME: should depends on user configuration
	}
	if(!mIceCtxVideo && (_eMediaType & tmedia_video)){
		mIceCtxVideo = tnet_ice_ctx_create(USE_ICE_JINGLE, USE_IPV6, USE_ICE_RTCP, TRUE/*video*/, &_PeerConnection::IceCallback, this);
		if(!mIceCtxVideo){
			TSK_DEBUG_ERROR("Failed to create ICE video context");
			return false;
		}
		tnet_ice_ctx_set_stun(mIceCtxVideo, "stun.l.google.com", 19302, "Doubango", "stun-username", "stun-password"); // FIXME: should depends on user configuration
	}

	// "none" comparison is used to exclude the "first call"
	if(mMediaType != tmedia_none && mMediaType != _eMediaType){
		// cancels contexts associated to old medias
		if(mIceCtxAudio && !(_eMediaType & tmedia_audio)){
			tnet_ice_ctx_cancel(mIceCtxAudio);
		}
		if(mIceCtxVideo && !(_eMediaType & tmedia_video)){
			tnet_ice_ctx_cancel(mIceCtxVideo);
		}
		// cancels contexts associated to new medias (e.g. session "remove" then "add")
		// cancel() on newly created contexts don't have any effect
		if(mIceCtxAudio && (!(_eMediaType & tmedia_audio) && (mMediaType & tmedia_audio))){
			//tnet_ice_ctx_cancel(mIceCtxAudio);
		}
		if(mIceCtxVideo && (!(_eMediaType & tmedia_video) && (mMediaType & tmedia_video))){
			//tnet_ice_ctx_cancel(mIceCtxVideo);
		}
	}

	mMediaType = _eMediaType;
	

	// For now disable timers until both parties get candidates
	// (RECV ACK) or RECV (200 OK)
	IceSetTimeout(-1);
	
	return true;
}

bool _PeerConnection::IceSetTimeout(int32_t timeout)
{
	if(mIceCtxAudio){
		tnet_ice_ctx_set_concheck_timeout(mIceCtxAudio, timeout);
	}
	if(mIceCtxVideo){
		tnet_ice_ctx_set_concheck_timeout(mIceCtxVideo, timeout);
	}
	return S_OK;
}

bool _PeerConnection::IceGotLocalCandidates()
{
	return IceGotLocalCandidates(mIceCtxAudio) && IceGotLocalCandidates(mIceCtxVideo);
}

bool _PeerConnection::IceGotLocalCandidates(struct tnet_ice_ctx_s *iceCtx)
{
	return (!tnet_ice_ctx_is_active(iceCtx) || tnet_ice_ctx_got_local_candidates(iceCtx));
}

bool _PeerConnection::IceProcessRo(const tsdp_message_t* sdp_ro, bool isOffer)
{
	if(!sdp_ro){
		TSK_DEBUG_ERROR("Invalid argument");
		return false;
	}

	char* ice_remote_candidates;
	const tsdp_header_M_t* M;
	tsk_size_t index;
	const tsdp_header_A_t *A;
	const char* sess_ufrag = tsk_null;
	const char* sess_pwd = tsk_null;
	int ret = 0, i;

	if(!sdp_ro){
		TSK_DEBUG_ERROR("Invalid argument");
		return false;
	}
	if(!mIceCtxAudio && !mIceCtxVideo){
		TSK_DEBUG_ERROR("Not ready yet");
		return false;
	}

	// session level attributes
	
	if((A = tsdp_message_get_headerA(sdp_ro, "ice-ufrag"))){
		sess_ufrag = A->value;
	}
	if((A = tsdp_message_get_headerA(sdp_ro, "ice-pwd"))){
		sess_pwd = A->value;
	}
	
#if 0 // Use RTCWeb Profile (tmedia_profile_rtcweb)
	{
		const tsdp_header_S_t *S;
		if((S = (const tsdp_header_S_t *)tsdp_message_get_header(sdp_ro, tsdp_htype_S)) && S->value){
			self->ice.is_jingle = tsk_strcontains(S->value, tsk_strlen(S->value), "webrtc");
		}
	}
#endif
	
	for(i = 0; i < 2; ++i){
		if((M = tsdp_message_find_media(sdp_ro, i==0 ? "audio": "video"))){
			const char *ufrag = sess_ufrag, *pwd = sess_pwd;
			ice_remote_candidates = tsk_null;
			index = 0;
			if((A = tsdp_header_M_findA(M, "ice-ufrag"))){
				ufrag = A->value;
			}
			if((A = tsdp_header_M_findA(M, "ice-pwd"))){
				pwd = A->value;
			}

			while((A = tsdp_header_M_findA_at(M, "candidate", index++))){
				tsk_strcat_2(&ice_remote_candidates, "%s\r\n", A->value);
			}
			// ICE processing will be automatically stopped if the remote candidates are not valid
			// ICE-CONTROLLING role if we are the offerer
			ret = tnet_ice_ctx_set_remote_candidates(i==0 ? mIceCtxAudio : mIceCtxVideo, ice_remote_candidates, ufrag, pwd, !isOffer, USE_ICE_JINGLE);
			TSK_SAFE_FREE(ice_remote_candidates);
		}
	}

	return (ret == 0);
}

bool _PeerConnection::IceIsDone()
{
	return (!tnet_ice_ctx_is_active(mIceCtxAudio) || tnet_ice_ctx_is_connected(mIceCtxAudio))
		&& (!tnet_ice_ctx_is_active(mIceCtxVideo) || tnet_ice_ctx_is_connected(mIceCtxVideo));
}

bool _PeerConnection::IceIsEnabled(const struct tsdp_message_s* sdp)
{
	bool isEnabled = false;
	if(sdp) {
		int i = 0;
		const tsdp_header_M_t* M;
		while((M = (tsdp_header_M_t*)tsdp_message_get_headerAt(sdp, tsdp_htype_M, i++))) {
			isEnabled = true; // at least one "candidate"
			if(!tsdp_header_M_findA(M, "candidate")) {
				return false;
			}
		}
	}
	
	return isEnabled;
}

int _PeerConnection::IceCallback(const tnet_ice_event_t *e)
{
	int ret = 0;
	_PeerConnection *This;

	This = (_PeerConnection *)e->userdata;

	EnterCriticalSection(&This->mCSIceCallback);

	TSK_DEBUG_INFO("W4A::_PeerConnection::ICE callback: %s", e->phrase);

	switch(e->type){
		case tnet_ice_event_type_started:
			{
				This->mIceState = IceStateGathering;
				break;
			}

		case tnet_ice_event_type_gathering_completed:
		case tnet_ice_event_type_conncheck_succeed:
		case tnet_ice_event_type_conncheck_failed:
		case tnet_ice_event_type_cancelled:
			{
				if(e->type == tnet_ice_event_type_gathering_completed){
					char* candidateStr = NULL;
					const char* ufragStr = tnet_ice_ctx_get_ufrag(e->ctx);
					const char* pwdStr = tnet_ice_ctx_get_pwd(e->ctx);
					bool bGotAllCandidates = This->IceGotLocalCandidates();
					const char* mediaStr = (e->ctx == This->mIceCtxAudio) ? "audio" : "video";
					const tsdp_header_M_t* M = tsdp_message_find_media(This->mSdpLocal, mediaStr);
					tnet_ice_candidate_t* pc_candidate;
					tsk_size_t nIceCandidatesCount = tnet_ice_ctx_count_local_candidates(e->ctx);
					for(USHORT index = 0; index < nIceCandidatesCount; ++index){
						candidateStr = tsk_strdup(tnet_ice_candidate_tostring((pc_candidate = ((tnet_ice_candidate_t*)tnet_ice_ctx_get_local_candidate_at(e->ctx, index)))));
						if(M){
							tsdp_header_M_add_headers((tsdp_header_M_t*)M, TSDP_HEADER_A_VA_ARGS("candidate", candidateStr), tsk_null);
							if (M->port == 0) {
								TSDP_HEADER_M(M)->port = pc_candidate->port;
							}
						}
						tsk_strcat_2(&candidateStr, " webrtc4ie-ufrag %s webrtc4ie-pwd %s", ufragStr, pwdStr);
						bool bMoreToFollow = !(bGotAllCandidates && (index == (nIceCandidatesCount - 1)));
						if(This->GetWindowHandle()){
							PeerConnectionEvent* oEvent = new PeerConnectionEvent(mediaStr, candidateStr, bMoreToFollow);
							if(!PostMessageA((HWND)This->GetWindowHandle(), WM_ICE_EVENT_CANDIDATE, reinterpret_cast<WPARAM>(This), reinterpret_cast<LPARAM>(oEvent))){
								TSK_DEBUG_ERROR("PostMessageA() failed");
								This->IceCallbackFire(oEvent);
								if(oEvent) delete oEvent, oEvent = NULL;
							}
						}				

						TSK_FREE(candidateStr);
					}

					if(bGotAllCandidates){
						This->mIceState = IceStateCompleted;
					}
				}
				else if(e->type == tnet_ice_event_type_conncheck_succeed){
					if(This->IceIsDone()){
						This->mIceState = IceStateConnected;
						if(This->GetWindowHandle()){
							PostMessageA((HWND)This->GetWindowHandle(), WM_ICE_EVENT_CONNECTED, reinterpret_cast<WPARAM>(This), NULL);
							//if(This->mStartDelayedUntilIceDone) {
							//	This->StartMedia();// Start() must not be called on worker thread
							//}
						}
					}
				}
				else if(e->type == tnet_ice_event_type_conncheck_failed || e->type == tnet_ice_event_type_cancelled){
					// "tnet_ice_event_type_cancelled" means remote party doesn't support ICE -> Not an error
					This->mIceState = (e->type == tnet_ice_event_type_conncheck_failed) ? IceStateFailed : IceStateClosed;
					if(This->GetWindowHandle()){
						PostMessageA((HWND)This->GetWindowHandle(), (e->type == tnet_ice_event_type_conncheck_failed) ? WM_ICE_EVENT_FAILED : WM_ICE_EVENT_CANCELLED, reinterpret_cast<WPARAM>(This), NULL);
						//if(This->mStartDelayedUntilIceDone) {
						//	This->StartMedia();// Start() must not be called on worker thread
						//}
					}
					if(e->type == tnet_ice_event_type_cancelled) {
						This->SignalNoMoreIceCandidateToFollow();
					}
				}
				break;
			}

		// fatal errors which discard ICE process
		case tnet_ice_event_type_gathering_host_candidates_failed:
		case tnet_ice_event_type_gathering_reflexive_candidates_failed:
			{
				This->mIceState = IceStateFailed;
				break;
			}
	}

	LeaveCriticalSection(&This->mCSIceCallback);

	return ret;
}

// callback function called when media session events (related to rfc5168) occures asynchronously
int _PeerConnection::Rfc5168Callback(const void* usrdata, const struct tmedia_session_s* session, const char* reason, enum tmedia_session_rfc5168_cmd_e command)
{
	int ret = 0;
	_PeerConnection *This;

	This = (_PeerConnection *)usrdata;
	
	TSK_DEBUG_INFO("W4A::_PeerConnection::Rfc5168Callback: %d-%s", command, reason);
	
	if (This) {
		if (command == tmedia_session_rfc5168_cmd_picture_fast_update) {
			if (This->GetWindowHandle()) {
				static const char* __command_picture_fast_update = "picture_fast_update";
				PostMessageA((HWND)This->GetWindowHandle(), WM_RFC5168_EVENT, reinterpret_cast<WPARAM>(This), reinterpret_cast<LPARAM>(__command_picture_fast_update));
			}
			else {
				TSK_DEBUG_ERROR("No handle");
			}
		}
	}
	return 0;
}