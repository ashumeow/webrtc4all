#include "tinymedia.h"

#include "tsk_debug.h"

#include "GothamProducers.h"
#include "GothamSessionCall.h"

#include <assert.h>

/*
* Converts the media type from local (SinCity) to native (Doubango).
*/
static tmedia_type_t _mediaTypeToNative(GmMediaType_t mediaType)
{
	tmedia_type_t type = tmedia_none;
	if (mediaType & GmMediaType_Audio) {
		type = (tmedia_type_t)(type | tmedia_audio);
	}
	if (mediaType & GmMediaType_Video) {
		type = (tmedia_type_t)(type | tmedia_video);
	}
	if (mediaType & GmMediaType_ScreenCast) {
		type = (tmedia_type_t)(type | tmedia_bfcp_video);
	}
	return type;
}

/*
* Converts the media type from native (Doubango) to local (SinCity).
*/
static GmMediaType_t _mediaTypeFromNative(tmedia_type_t mediaType)
{
	GmMediaType_t type = GmMediaType_None;
	if (mediaType & tmedia_audio) {
		type = (GmMediaType_t)(type | GmMediaType_Audio);
	}
	if (mediaType & tmedia_video) {
		type = (GmMediaType_t)(type | GmMediaType_Video);
	}
	if (mediaType & tmedia_bfcp_video) {
		type = (GmMediaType_t)(type | GmMediaType_ScreenCast);
	}
	return type;
}

GmSessionCall::GmSessionCall()
	: GmSession(GmSessionType_Call)
	, m_eMediaType(GmMediaType_Video) // Only video media type is supported as stream
	, m_bNattIceEnabled(tmedia_defaults_get_ice_enabled() == tsk_true || tmedia_defaults_get_profile() == tmedia_profile_rtcweb)
	, m_bNattIceStunEnabled(tmedia_defaults_get_icestun_enabled() == tsk_true)
	, m_bNattIceTurnEnabled(tmedia_defaults_get_iceturn_enabled() == tsk_true)
	, m_bRTCPEnabled(tmedia_defaults_get_rtcp_enabled() == tsk_true)
	, m_bRTCPMuxEnabled(tmedia_defaults_get_rtcpmux_enabled() == tsk_true)
	, m_eAVPFMode((tmedia_defaults_get_profile() == tmedia_profile_rtcweb) ? GmMode_Mandatory : (GmMode_t)tmedia_defaults_get_avpf_mode())
	, m_VideoDisplayLocal(NULL)
	, m_VideoDisplayRemote(NULL)
	, m_ScreenCastDisplayLocal(NULL)
	, m_ScreenCastDisplayRemote(NULL)
	, m_pProducer(NULL)
	, m_guidInputFormat(GUID_NULL)
	, m_nInputWidth(0)
	, m_nInputHeight(0)
	, m_bStartDeferred(false)
	, m_bStarted(false)
{
	m_objMutex = GmObj::NewArg0<GmMutex>();
	GM_ASSERT(m_objMutex != NULL);
}

GmSessionCall::~GmSessionCall()
{
	Cleanup();
	m_objMutex = NULL;
	m_listIceServers.clear();
	GmSafeRelease(&m_pProducer);

	GM_DEBUG_INFO("*** GmSessionCall destroyed ***");
}


/**
* Starts the media sessions
*/
HRESULT GmSessionCall::Start()
{
	HRESULT hr = S_OK;

	GmAutoLock<GmSessionCall> autoLock(this);

	if (m_bStarted)
	{
		GM_DEBUG_WARN("Already started");
		goto bail;
	}

	if (m_bNattIceEnabled && !IceIsDone())
	{
		GM_DEBUG_WARN("ICE not connected yet...deferring start");
		m_bStartDeferred = true;
		goto bail;
	}
	if (!m_pSessionMgr)
	{
		GM_CHECK_HR(hr = E_ILLEGAL_METHOD_CALL);
	}

	GM_CHECK_HR(hr = AttachVideoDisplays());

	if (tmedia_session_mgr_start(m_pSessionMgr) != 0)
	{
		GM_CHECK_HR(hr = E_FAIL);
	}

	m_bStarted = true;

bail:
	return hr;
}

/**
* Pauses the media sessions
*/
HRESULT GmSessionCall::Pause()
{
	HRESULT hr = S_OK;

	GmAutoLock<GmSessionCall> autoLock(this);

	GM_DEBUG_WARN("Pause not implemented yet");

	return hr;
}

/**
* Stops the media sessions
*/
HRESULT GmSessionCall::Stop()
{
	HRESULT hr = S_OK;

	GmAutoLock<GmSessionCall> autoLock(this);

	if (m_bStarted)
	{
		if (m_pSessionMgr)
		{
			tmedia_session_mgr_stop(m_pSessionMgr);
		}

		if (m_pIceCtxScreenCast)
		{
			tnet_ice_ctx_stop(m_pIceCtxScreenCast);
		}
		if (m_pIceCtxVideo)
		{
			tnet_ice_ctx_stop(m_pIceCtxVideo);
		}
		if (m_pIceCtxAudio)
		{
			tnet_ice_ctx_stop(m_pIceCtxAudio);
		}
	}

	// Producer will be created next time we try to send samples
	GmSafeRelease(&m_pProducer);

	m_bStarted = false;
	m_bStartDeferred = false;

	return hr;
}

void GmSessionCall::Lock()
{
	m_objMutex->Lock();
}

void GmSessionCall::Unlock()
{
	m_objMutex->Unlock();
}

HRESULT GmSessionCall::Cleanup()
{
	GmAutoLock<GmSessionCall> autoLock(this);	

	Stop();

	TSK_OBJECT_SAFE_FREE(m_pSessionMgr);

	TSK_OBJECT_SAFE_FREE(m_pIceCtxScreenCast);
	TSK_OBJECT_SAFE_FREE(m_pIceCtxVideo);
	TSK_OBJECT_SAFE_FREE(m_pIceCtxAudio);

	GmSafeRelease(&m_pProducer);

	m_strLocalSdpType = "";

	return S_OK;
}

HRESULT GmSessionCall::SetInputSize(UINT32 nWidth, UINT32 nHeight)
{
	HRESULT hr = S_OK;
	GmAutoLock<GmSessionCall> autoLock(this);

	if (m_pProducer)
	{
		GM_CHECK_HR(hr = m_pProducer->SetInputSize(nWidth, nHeight));
	}
	m_nInputWidth = nWidth;
	m_nInputHeight = nHeight;

bail:
	return hr;
}

HRESULT GmSessionCall::SetInputFormat(const GUID& subType)
{
	HRESULT hr = S_OK;
	GmAutoLock<GmSessionCall> autoLock(this);

	if (m_pProducer)
	{
		GM_CHECK_HR(hr = m_pProducer->SetInputFormat(subType));
	}
	m_guidInputFormat = subType;

bail:
	return hr;
}

/**@ingroup _Group_CPP_SessionCall
* Sets the video display where to draw the frames.
* @param eVideoType The video type for which to set the displays. Must be @ref SCMediaType_Video or @ref SCMediaType_ScreenCast.
* @param displayLocal The local display (a.k.a 'preview').
* @param displayRemote The remote display (where to draw frames sent by the remote peer).
* @retval HRESULT
*/
HRESULT GmSessionCall::SetVideoDisplays(GmMediaType_t eVideoType, HWND displayLocal /*= NULL*/, HWND displayRemote /*= NULL*/)
{
	HRESULT hr = S_OK;
	GmAutoLock<GmSessionCall> autoLock(this);
	HWND *pLocal, *pRemote;

	if (eVideoType != GmMediaType_Video && eVideoType != GmMediaType_ScreenCast)
	{
		GM_CHECK_HR(hr = E_INVALIDARG);
	}
	pLocal = (eVideoType == GmMediaType_Video) ? &m_VideoDisplayLocal : &m_ScreenCastDisplayLocal;
	pRemote = (eVideoType == GmMediaType_Video) ? &m_VideoDisplayRemote : &m_ScreenCastDisplayRemote;

	*pLocal = displayLocal;
	*pRemote = displayRemote;

	GM_CHECK_HR(hr = AttachVideoDisplays());

bail:
	return hr;
}

HRESULT GmSessionCall::GetLocalSDP(std::string &strSDP)
{
	HRESULT hr = S_OK;
	const tsdp_message_t* sdp_lo;
	char* sdpStr = tsk_null;

	GmAutoLock<GmSessionCall> autoLock(this);
	
	if (!m_pSessionMgr)
	{
		// Set local SDP type
		m_strLocalSdpType = "offer";

		// Create local offer
		GM_CHECK_HR(hr = CreateLocalOffer());
	}

	if (m_bNattIceEnabled && !IceGotLocalCandidatesAll())
	{
		GM_DEBUG_ERROR("ICE gathering not done");
		GM_CHECK_HR(hr = E_ILLEGAL_METHOD_CALL);
	}

	// Get local now that ICE gathering is done
	sdp_lo = tmedia_session_mgr_get_lo(m_pSessionMgr);
	if (!sdp_lo)
	{
		GM_DEBUG_ERROR("Cannot get local offer");
		GM_CHECK_HR(hr = E_FAIL);
	}

	sdpStr = tsdp_message_tostring(sdp_lo);
	if (tsk_strnullORempty(sdpStr))
	{
		GM_DEBUG_ERROR("Cannot serialize local offer");
		GM_CHECK_HR(hr = E_FAIL);
	}
	strSDP = std::string(sdpStr);

bail:
	TSK_FREE(sdpStr);
	return hr;
}

HRESULT GmSessionCall::SetRemoteSDP(const std::string &strSDP)
{
	HRESULT hr = S_OK;
	tsdp_message_t* pSdp_ro = tsk_null;
	bool bRemoteIsOffer;
	int iRet = 0;
	tmedia_ro_type_t ro_type;
	GmMediaType_t newMediaType;

	GmAutoLock<GmSessionCall> autoLock(this);

	if (strSDP.empty())
	{
		GM_CHECK_HR(hr = E_INVALIDARG);
	}

	bRemoteIsOffer = (m_strLocalSdpType != "offer");

	// Parse SDP
	if (!(pSdp_ro = tsdp_message_parse(strSDP.c_str(), strSDP.length())))
	{
		GM_DEBUG_ERROR("Failed to parse SDP: %s", strSDP.c_str());
		GM_CHECK_HR(hr = E_INVALIDARG);
	}

	m_bNattIceEnabled &= !IceIsEnabled(pSdp_ro);
	
	ro_type = bRemoteIsOffer ? tmedia_ro_type_answer : tmedia_ro_type_offer;

	newMediaType = _mediaTypeFromNative(tmedia_type_from_sdp(pSdp_ro));

	// In this version only Video is supported
	if (newMediaType != m_eMediaType)
	{
		GM_DEBUG_ERROR("Media type(%d) not supported", newMediaType);
		GM_CHECK_HR(hr = E_INVALIDARG);
	}

	GM_DEBUG_INFO("New mediaType=%d", newMediaType);
	
	if (m_eMediaType != newMediaType)
	{
		GM_DEBUG_INFO("Media type mismatch: offer=%d,reponse=%d", m_eMediaType, newMediaType);
		// Cleanup ICE contexes
		if (!(newMediaType & GmMediaType_Audio) && m_pIceCtxAudio)
		{
			GM_DEBUG_INFO("Destroying ICE audio context");
			TSK_OBJECT_SAFE_FREE(m_pIceCtxAudio);
			if (m_pSessionMgr)
			{
				tmedia_session_mgr_set_ice_ctx_2(m_pSessionMgr, tmedia_audio, tsk_null);
			}
		}
		if (!(newMediaType & GmMediaType_Video) && m_pIceCtxVideo)
		{
			GM_DEBUG_INFO("Destroying ICE video context");
			TSK_OBJECT_SAFE_FREE(m_pIceCtxVideo);
			if (m_pSessionMgr)
			{
				tmedia_session_mgr_set_ice_ctx_2(m_pSessionMgr, tmedia_video, tsk_null);
			}
		}
		if (!(newMediaType & GmMediaType_ScreenCast) && m_pIceCtxScreenCast)
		{
			GM_DEBUG_INFO("Destroying ICE screencast context");
			TSK_OBJECT_SAFE_FREE(m_pIceCtxScreenCast);
			if (m_pSessionMgr)
			{
				tmedia_session_mgr_set_ice_ctx_2(m_pSessionMgr, tmedia_bfcp_video, tsk_null);
			}
		}
		m_eMediaType = newMediaType;
	}

	GM_CHECK_HR(hr = CreateLocalOffer(pSdp_ro, ro_type));

	// Start session manager if ICE done and not already started
	if (m_bNattIceEnabled && IceIsDone())
	{
		GM_CHECK_HR(hr = IceProcessRo(pSdp_ro, (ro_type == tmedia_ro_type_offer)));
	}

bail:
	TSK_OBJECT_SAFE_FREE(pSdp_ro);

	return hr;
}


/**@ingroup _Group_CPP_SessionCall
* Defines whether to enable ICE and gather host candidates.
* @param enabled
* @retval HRESULT
*/
HRESULT GmSessionCall::SetNattIceEnabled(bool bEnabled)
{
	m_bNattIceEnabled = bEnabled;
	return S_OK;
}

/**@ingroup _Group_CPP_SessionCall
* Defines whether to gather ICE reflexive candidates.
* @param enabled
* @retval HRESULT
*/
HRESULT GmSessionCall::SetNattIceStunEnabled(bool bEnabled)
{
	m_bNattIceStunEnabled = bEnabled;
	return S_OK;
}


/**@ingroup _Group_CPP_SessionCall
* Defines whether to gather ICE relay candidates.
* @param enabled
* @retval HRESULT
*/
HRESULT GmSessionCall::SetNattIceTurnEnabled(bool bEnabled)
{
	m_bNattIceTurnEnabled = bEnabled;
	return S_OK;
}


/**@ingroup _Group_CPP_SessionCall
* Defines whether to enabled RTCP.
* @param enabled
* @retval HRESULT
*/
HRESULT GmSessionCall::SetRTCPEnabled(bool bEnabled)
{
	m_bRTCPEnabled = bEnabled;
	return S_OK;
}

/**@ingroup _Group_CPP_SessionCall
* Defines whether to enabled RTCP muxing.
* @param enabled
* @retval HRESULT
*/
HRESULT GmSessionCall::SetRTCPMuxEnabled(bool bEnabled)
{
	m_bRTCPMuxEnabled = bEnabled;
	return S_OK;
}

/**@ingroup _Group_CPP_SessionCall
* Defines AVPF negotiation mode.
* @param eMode
* @retval HRESULT
*/
HRESULT GmSessionCall::SetAVPFMode(GmMode_t eMode)
{
	if (eMode != tmedia_mode_none && eMode != tmedia_mode_optional && eMode != tmedia_mode_mandatory)
	{
		return E_INVALIDARG;
	}
	GM_ASSERT(tmedia_defaults_set_avpf_mode((tmedia_mode_t)eMode) == 0);
	m_eAVPFMode = eMode;
	return S_OK;
}

/**@ingroup _Group_CPP_SessionCall
* Adds a new ICE server to try when gathering candidates. You can add as many servers as you want. The servers will be tried in FIFT (First In First to Try) order. <br />
* Available since <b>1.0.1</b>.
* @param strTransportProto The transport protocol. Must be <b>udp</b>, <b>tcp</b> or <b>tls</b>. This parameter is case-insensitive.
* @param strServerHost ICE server hostname or IP address.
* @param serverPort ICE server port.
* @param useTurn Whether to use this ICE server to gather relayed candidates.
* @param useStun Whether to use this ICE server to gather reflexive candidates.
* @param strUsername Username used for long-term credentials. Required for TURN allocation.
* @param strPassword Password used for long-term credentials. Required for TURN allocation.
* @retval HRESULT
* @sa @ref ClearNattIceServers()
*/
HRESULT GmSessionCall::AddNattIceServer(std::string strTransportProto, std::string strServerHost, unsigned short serverPort, bool useTurn/* = false*/, bool useStun /*= true*/, std::string strUsername /*= ""*/, std::string strPassword /*= ""*/)
{
	HRESULT hr = S_OK;
	GmIceServer* pIceServer = NULL;

	GmAutoLock<GmSessionCall> autoLock(this);

	if (strTransportProto.empty() || strServerHost.empty() || !serverPort || (!useTurn && !useStun))
	{
		GM_CHECK_HR(hr = E_INVALIDARG);
	}
	if (!tsk_striequals(strTransportProto.c_str(), "udp") && !tsk_striequals(strTransportProto.c_str(), "tcp") && !tsk_striequals(strTransportProto.c_str(), "tls"))
	{
		GM_DEBUG_ERROR("'%s' not valid as ICE server transport protocol", strTransportProto);
		GM_CHECK_HR(hr = E_INVALIDARG);
	}

	pIceServer = new GmIceServer(
		strTransportProto,
		strServerHost,
		serverPort,
		useTurn,
		useStun,
		strUsername,
		strPassword);
	if (!pIceServer)
	{
		GM_CHECK_HR(hr = E_OUTOFMEMORY);
	}
	m_listIceServers.push_back(pIceServer);

bail:
	GmSafeRelease(&pIceServer);
	return hr;
}

/**
* Encode bytes and send over the network
*/
HRESULT GmSessionCall::WriteSamples(const BYTE* pcBytesPtr, UINT32 nBytesCount)
{
	HRESULT hr = S_OK;

	GmAutoLock<GmSessionCall> autoLock(this);

	if (!pcBytesPtr || !nBytesCount)
	{
		GM_CHECK_HR(hr = E_INVALIDARG);
	}

	if (!m_bStarted)
	{
		GM_DEBUG_ERROR("Media session manager not started yet ... cannot send data");
		GM_CHECK_HR(hr = E_ILLEGAL_METHOD_CALL);
	}

	if (!m_pProducer)
	{
		struct tmedia_producer_s* pWrappedProducer = tsk_null;
		tmedia_type_t nativeMediaType = _mediaTypeToNative(m_eMediaType);
		int ret = tmedia_session_mgr_get(m_pSessionMgr,
			TMEDIA_SESSION_GET_POBJECT(nativeMediaType, "producer", &pWrappedProducer),

			TMEDIA_SESSION_SET_NULL()
			);
		if (ret != 0)
		{
			TSK_OBJECT_SAFE_FREE(pWrappedProducer);
			GM_CHECK_HR(hr = E_FAIL);
		}
		if (!pWrappedProducer)
		{
			GM_CHECK_HR(hr = E_POINTER);
		}
		if (pWrappedProducer->plugin != gm_producer_plugin_def_t)
		{
			TSK_OBJECT_SAFE_FREE(pWrappedProducer);
			GM_CHECK_HR(hr = E_POINTER);
		}

		m_pProducer = new GmProducer((const struct gm_producer_s*)pWrappedProducer);

		TSK_OBJECT_SAFE_FREE(pWrappedProducer);

		if (!m_pProducer)
		{
			GM_CHECK_HR(hr = E_OUTOFMEMORY);
		}
		GM_CHECK_HR(hr = m_pProducer->SetInputFormat(m_guidInputFormat));
		GM_CHECK_HR(hr = m_pProducer->SetInputSize(m_nInputWidth, m_nInputHeight));
	}

	GM_CHECK_HR(hr = m_pProducer->WriteSamples(pcBytesPtr, nBytesCount));

bail:
	return hr;
}

CComPtr<GmSessionCall> GmSessionCall::New(const std::string &strRemoteSDP /*= ""*/)
{
	CComPtr<GmSessionCall>call = GmObj::NewArg0<GmSessionCall>();
	if (!call)
	{
		GM_DEBUG_ERROR("Failed to create call session");
		return NULL;
	}

	if (!strRemoteSDP.empty())
	{

	}

	return call;
}


//
//	Private functions
//


/*
* Creates a new media session manager.
* @retval HRESULT
*/
HRESULT GmSessionCall::CreateSessionMgr()
{
	HRESULT hr = S_OK;
	int iRet;
	tnet_ip_t bestsource;
	tmedia_type_t nativeMediaType;
	tsk_bool_t nativeRTCPEnabled, nativeRTCPMuxEnabled;

	GmAutoLock<GmSessionCall> autoLock(this);

	nativeMediaType = _mediaTypeToNative(m_eMediaType);

	TSK_OBJECT_SAFE_FREE(m_pSessionMgr);

	TSK_OBJECT_SAFE_FREE(m_pIceCtxScreenCast);
	TSK_OBJECT_SAFE_FREE(m_pIceCtxVideo);
	TSK_OBJECT_SAFE_FREE(m_pIceCtxAudio);

	// Create ICE contexts
	if (m_bNattIceEnabled)
	{
		GM_CHECK_HR(hr = IceCreateCtxAll());
	}

	if ((iRet = tnet_getbestsource("stun.l.google.com", 19302, tnet_socket_type_udp_ipv4, &bestsource)))
	{
		GM_DEBUG_ERROR("Failed to get best source [%d]", iRet);
		memcpy(bestsource, "0.0.0.0", 7);
	}

	// Create media session manager
	m_pSessionMgr = tmedia_session_mgr_create(nativeMediaType, bestsource, tsk_false/*IPv6*/, tsk_true/* offerer */);
	if (!m_pSessionMgr)
	{
		GM_CHECK_HR(hr = E_OUTOFMEMORY);
	}

	// Set ICE contexts
	if (tmedia_session_mgr_set_ice_ctx_2(m_pSessionMgr, _mediaTypeToNative(GmMediaType_Audio), m_pIceCtxAudio) != 0)
	{
		GM_CHECK_HR(hr = E_FAIL);
	}
	if (tmedia_session_mgr_set_ice_ctx_2(m_pSessionMgr, _mediaTypeToNative(GmMediaType_Video), m_pIceCtxVideo) != 0)
	{
		GM_CHECK_HR(hr = E_FAIL);
	}
	if (tmedia_session_mgr_set_ice_ctx_2(m_pSessionMgr, _mediaTypeToNative(GmMediaType_ScreenCast), m_pIceCtxScreenCast) != 0)
	{
		GM_CHECK_HR(hr = E_FAIL);
	}

	nativeRTCPEnabled = m_bRTCPEnabled ? tsk_true : tsk_false;
	nativeRTCPMuxEnabled = m_bRTCPMuxEnabled ? tsk_true : tsk_false;
	tmedia_session_mgr_set(m_pSessionMgr,
		TMEDIA_SESSION_SET_INT32(nativeMediaType, "avpf-mode", m_eAVPFMode),
		TMEDIA_SESSION_SET_INT32(nativeMediaType, "rtcp-enabled", nativeRTCPEnabled),
		TMEDIA_SESSION_SET_INT32(nativeMediaType, "rtcpmux-enabled", nativeRTCPMuxEnabled),

		TMEDIA_SESSION_SET_NULL()
		);

	GM_CHECK_HR(hr = AttachVideoDisplays());

bail:
	return hr;
}

/*
* Creates a new local SDP offer.
* @param pc_RO Remote offer.
* @param _eRoType Remote offer type.
* @retval HRESULT
*/
HRESULT GmSessionCall::CreateLocalOffer(const struct tsdp_message_s* pc_Ro /*= NULL*/, GmRoType _eRoType /*= 0 = None*/)
{
	HRESULT hr = S_OK;
	enum tmedia_ro_type_e eRoType = (enum tmedia_ro_type_e)_eRoType;

	GmAutoLock<GmSessionCall> autoLock(this);

	if (!m_pSessionMgr)
	{
		GM_CHECK_HR(hr = CreateSessionMgr());
	}
	else
	{
		// update media type
		if (tmedia_session_mgr_set_media_type(m_pSessionMgr, _mediaTypeToNative(m_eMediaType)) != 0)
		{
			GM_DEBUG_ERROR("Failed to update media type %d->%d", m_pSessionMgr->type, _mediaTypeToNative(m_eMediaType));
			GM_CHECK_HR(hr = E_FAIL);
		}
	}

	if (pc_Ro)
	{
		if (tmedia_session_mgr_set_ro(m_pSessionMgr, pc_Ro, eRoType) != 0)
		{
			GM_CHECK_HR(hr = E_FAIL);
		}
		if (m_bNattIceEnabled)
		{
			GM_CHECK_HR(hr = IceProcessRo(pc_Ro, (eRoType == tmedia_ro_type_offer)));
		}
	}

	m_strLocalSdpType = (eRoType == tmedia_ro_type_offer) ? "answer" : "offer";

	// Start ICE
	if (m_bNattIceEnabled)
	{
		GM_CHECK_HR(hr = IceStart());
	}

bail:
	return hr;
}

HRESULT GmSessionCall::AttachVideoDisplays()
{
	GmAutoLock<GmSessionCall> autoLock(this);

	if (m_pSessionMgr)
	{
		if ((m_pSessionMgr->type & tmedia_video) == tmedia_video)
		{
			tmedia_session_mgr_set(m_pSessionMgr,
				TMEDIA_SESSION_PRODUCER_SET_INT64(tmedia_video, "local-hwnd", /*reinterpret_cast<int64_t>*/(m_VideoDisplayLocal)),
				TMEDIA_SESSION_CONSUMER_SET_INT64(tmedia_video, "remote-hwnd", /*reinterpret_cast<int64_t>*/(m_VideoDisplayRemote)),

				TMEDIA_SESSION_SET_NULL());
		}
		if ((m_pSessionMgr->type & tmedia_bfcp_video) == tmedia_bfcp_video)
		{
			static void* __entireScreen = NULL;
			tmedia_session_mgr_set(m_pSessionMgr,
				TMEDIA_SESSION_PRODUCER_SET_INT64(tmedia_bfcp_video, "local-hwnd", /*reinterpret_cast<int64_t>*/(m_ScreenCastDisplayLocal)),
				TMEDIA_SESSION_PRODUCER_SET_INT64(tmedia_bfcp_video, "src-hwnd", /*reinterpret_cast<int64_t>*/(__entireScreen)),
				// The BFCP session is not expected to receive any media but Radvision use it as receiver for the mixed stream.
				TMEDIA_SESSION_CONSUMER_SET_INT64(tmedia_bfcp_video, "remote-hwnd", /*reinterpret_cast<int64_t>*/(m_ScreenCastDisplayRemote)),

				TMEDIA_SESSION_SET_NULL());
		}
	}
	return S_OK;
}

struct tnet_ice_ctx_s* GmSessionCall::IceCreateCtx(bool bVideo)
{
	GmAutoLock<GmSessionCall> autoLock(this);

	static tsk_bool_t __use_ice_jingle = tsk_false;
	static tsk_bool_t __use_ipv6 = tsk_false;
	static tsk_bool_t __use_ice_rtcp = tsk_true;

	struct tnet_ice_ctx_s* p_ctx = tsk_null;
	const char* ssl_priv_path = tsk_null;
	const char* ssl_pub_path = tsk_null;
	const char* ssl_ca_path = tsk_null;
	tsk_bool_t ssl_verify = tsk_false;
	GM_ASSERT(tmedia_defaults_get_ssl_certs(&ssl_priv_path, &ssl_pub_path, &ssl_ca_path, &ssl_verify) == 0);

	if ((p_ctx = tnet_ice_ctx_create(__use_ice_jingle, __use_ipv6, __use_ice_rtcp, bVideo ? tsk_true : tsk_false, &GmSessionCall::IceCallback, this)))
	{
		// Add ICE servers
		for (std::list<CComPtr<GmIceServer> >::const_iterator it = m_listIceServers.begin(); it != m_listIceServers.end(); ++it)
		{
			tnet_ice_ctx_add_server(
				p_ctx,
				(*it)->getTransport(),
				(*it)->getServerHost(),
				(*it)->getServerPort(),
				(*it)->isTurnEnabled(),
				(*it)->isStunEnabled(),
				(*it)->getUsername(),
				(*it)->getPassword());
		}
		// Set SSL certificates
		tnet_ice_ctx_set_ssl_certs(p_ctx, ssl_priv_path, ssl_pub_path, ssl_ca_path, ssl_verify);
		// Enable/Disable TURN/STUN
		tnet_ice_ctx_set_stun_enabled(p_ctx, m_bNattIceStunEnabled ? tsk_true : tsk_false);
		tnet_ice_ctx_set_turn_enabled(p_ctx, m_bNattIceTurnEnabled ? tsk_true : tsk_false);
	}
	return p_ctx;
}

/*
* Creates the ICE contexts. There will be as meany contexts as sessions (one per RTP session).
* @retval HRESULT
*/
HRESULT GmSessionCall::IceCreateCtxAll()
{
	HRESULT hr = S_OK;

	GmAutoLock<GmSessionCall> autoLock(this);

	if (!m_pIceCtxAudio && (m_eMediaType & GmMediaType_Audio))
	{
		if (!(m_pIceCtxAudio = IceCreateCtx(false/*audio*/)))
		{
			GM_DEBUG_ERROR("Failed to create ICE audio context");
			GM_CHECK_HR(hr = E_FAIL);
		}
	}
	if (!m_pIceCtxVideo && (m_eMediaType & GmMediaType_Video))
	{
		if (!(m_pIceCtxVideo = IceCreateCtx(true/*video*/)))
		{
			GM_DEBUG_ERROR("Failed to create ICE video context");
			GM_CHECK_HR(hr = E_FAIL);
		}
	}
	if (!m_pIceCtxScreenCast && (m_eMediaType & GmMediaType_ScreenCast))
	{
		if (!(m_pIceCtxScreenCast = IceCreateCtx(true/*video*/)))
		{
			GM_DEBUG_ERROR("Failed to create ICE screencast context");
			GM_CHECK_HR(hr = E_FAIL);
		}
	}

	// For now disable timers until both parties get candidates
	// (RECV ACK) or RECV (200 OK)
	GM_CHECK_HR(hr = IceSetTimeout(-1));

bail:
	return hr;
}


/*
* Sets the ICE timeouts.
* @param timeout Timeout value. Negative number means endless.
* @retval HRESULT
*/
HRESULT GmSessionCall::IceSetTimeout(INT32 timeout)
{
	GmAutoLock<GmSessionCall> autoLock(this);

	if (m_pIceCtxAudio)
	{
		tnet_ice_ctx_set_concheck_timeout(m_pIceCtxAudio, timeout);
	}
	if (m_pIceCtxVideo)
	{
		tnet_ice_ctx_set_concheck_timeout(m_pIceCtxVideo, timeout);
	}
	if (m_pIceCtxScreenCast)
	{
		tnet_ice_ctx_set_concheck_timeout(m_pIceCtxScreenCast, timeout);
	}
	return S_OK;
}

/*
* Checks whether gathering ICE candidates (host, reflexive and relayed) for a context is done.
* @param p_IceCtx ICE context for which to check the gathering state.
* @retval <b>true</b> if done; otherwise <b>false</b>.
*/
bool GmSessionCall::IceGotLocalCandidates(struct tnet_ice_ctx_s *p_IceCtx)
{
	GmAutoLock<GmSessionCall> autoLock(this);

	return (!tnet_ice_ctx_is_active(p_IceCtx) || tnet_ice_ctx_got_local_candidates(p_IceCtx));
}


/*
* Checks whether gathering ICE candidates (host, reflexive and relayed) for all contexts are done.
* @retval <b>true</b> if done; otherwise <b>false</b>.
*/
bool GmSessionCall::IceGotLocalCandidatesAll()
{
	GmAutoLock<GmSessionCall> autoLock(this);
	return IceGotLocalCandidates(m_pIceCtxAudio) && IceGotLocalCandidates(m_pIceCtxVideo) && IceGotLocalCandidates(m_pIceCtxScreenCast);
}

/*
* Process the SDP sent by the remote peer.
* @param pc_SdpRo the SDP.
* @param isOffer Whether it's an offer (<b>true</b>:offer, <b>false</b>:answer).
* @retval HRESULT
*/
HRESULT GmSessionCall::IceProcessRo(const struct tsdp_message_s* pc_SdpRo, bool isOffer)
{
	char* ice_remote_candidates;
	const tsdp_header_M_t *M_ro, *M_lo;
	tsk_size_t index0 = 0, index1;
	const tsdp_header_A_t *A;
	const char* sess_ufrag = tsk_null;
	const char* sess_pwd = tsk_null;
	int ret = 0;
	tmedia_type_t mt_ro, mt_lo;

	HRESULT hr = S_OK;

	GmAutoLock<GmSessionCall> autoLock(this);

	if (!pc_SdpRo)
	{
		GM_CHECK_HR(hr = E_INVALIDARG);
	}

	if (!m_pIceCtxAudio && !m_pIceCtxVideo && !m_pIceCtxScreenCast)
	{
		GM_DEBUG_ERROR("Not ready yet");
		GM_CHECK_HR(hr = E_ILLEGAL_STATE_CHANGE);
	}

	// session level attributes

	if ((A = tsdp_message_get_headerA(pc_SdpRo, "ice-ufrag")))
	{
		sess_ufrag = A->value;
	}
	if ((A = tsdp_message_get_headerA(pc_SdpRo, "ice-pwd")))
	{
		sess_pwd = A->value;
	}

	while ((M_ro = (const tsdp_header_M_t*)tsdp_message_get_headerAt(pc_SdpRo, tsdp_htype_M, index0++)))
	{
		struct tnet_ice_ctx_s * _ctx = tsk_null;
		M_lo = (m_pSessionMgr && m_pSessionMgr->sdp.lo) ? (const tsdp_header_M_t*)tsdp_message_get_headerAt(m_pSessionMgr->sdp.lo, tsdp_htype_M, (index0 - 1)) : tsk_null;
		mt_ro = tmedia_type_from_sdp_headerM(M_ro);
		mt_lo = M_lo ? tmedia_type_from_sdp_headerM(M_lo) : mt_ro;
		if (mt_lo != mt_ro && mt_lo == tmedia_bfcp_video)
		{
			GM_DEBUG_INFO("Patching remote media type from 'video' to 'bfcpvid'");
			mt_ro = tmedia_bfcp_video;
		}
		switch (mt_ro) {
		case tmedia_audio:
			_ctx = m_pIceCtxAudio;
			break;
		case tmedia_video:
			_ctx = m_pIceCtxVideo;
			break;
		case tmedia_bfcp_video:
			_ctx = m_pIceCtxScreenCast;
			break;
		default:
			continue;
		}

		if (!_ctx)
		{
			GM_DEBUG_INFO("ignoring ICE candidates from media type=%d", mt_ro);
			continue;
		}

		const char *ufrag = sess_ufrag, *pwd = sess_pwd;
		ice_remote_candidates = tsk_null;
		if ((A = tsdp_header_M_findA(M_ro, "ice-ufrag")))
		{
			ufrag = A->value;
		}
		if ((A = tsdp_header_M_findA(M_ro, "ice-pwd")))
		{
			pwd = A->value;
		}

		index1 = 0;
		while ((A = tsdp_header_M_findA_at(M_ro, "candidate", index1++)))
		{
			tsk_strcat_2(&ice_remote_candidates, "%s\r\n", A->value);
		}
		// ICE processing will be automatically stopped if the remote candidates are not valid
		// ICE-CONTROLLING role if we are the offerer
		ret = tnet_ice_ctx_set_remote_candidates(_ctx, ice_remote_candidates, ufrag, pwd, !isOffer, tsk_false/*Jingle?*/);
		TSK_SAFE_FREE(ice_remote_candidates);
	}

bail:
	return hr;
}

/*
* Checks that both ICE gathering and connection checks are dones.
* @retval <b>true</b> if done; otherwise <b>false</b>.
*/
bool GmSessionCall::IceIsDone()
{
	GmAutoLock<GmSessionCall> autoLock(this);

	return (!tnet_ice_ctx_is_active(m_pIceCtxAudio) || tnet_ice_ctx_is_connected(m_pIceCtxAudio))
		&& (!tnet_ice_ctx_is_active(m_pIceCtxVideo) || tnet_ice_ctx_is_connected(m_pIceCtxVideo))
		&& (!tnet_ice_ctx_is_active(m_pIceCtxScreenCast) || tnet_ice_ctx_is_connected(m_pIceCtxScreenCast));
}


/*
* Checks whether ICE is enabled.
* @param pc_Sdp SDP sent by the remote peer for which to check if ICE is enabled (looks for SDP "a=candidate" candidates).
* @retval <b>true</b> if enabled; otherwise <b>false</b>.
*/
bool GmSessionCall::IceIsEnabled(const struct tsdp_message_s* pc_Sdp)
{
	GmAutoLock<GmSessionCall> autoLock(this);

	bool isEnabled = false;
	if (pc_Sdp)
	{
		int i = 0;
		const tsdp_header_M_t* M;
		while ((M = (tsdp_header_M_t*)tsdp_message_get_headerAt(pc_Sdp, tsdp_htype_M, i++)))
		{
			isEnabled = true; // at least one "candidate"
			if (M->port != 0 && !tsdp_header_M_findA(M, "candidate"))
			{
				return false;
			}
		}
	}

	return isEnabled;
}


/*
* Starts the ICE contexts.
* @retval HRESULT
*/
HRESULT GmSessionCall::IceStart()
{
	int iRet;
	HRESULT hr = S_OK;

	GmAutoLock<GmSessionCall> autoLock(this);

	if ((m_eMediaType & GmMediaType_Audio) == GmMediaType_Audio)
	{
		if (m_pIceCtxAudio && (iRet = tnet_ice_ctx_start(m_pIceCtxAudio)) != 0)
		{
			GM_DEBUG_WARN("tnet_ice_ctx_start() failed with error code = %d", iRet);
			GM_CHECK_HR(hr = E_FAIL);
		}
	}
	if ((m_eMediaType & GmMediaType_Video) == GmMediaType_Video)
	{
		if (m_pIceCtxVideo && (iRet = tnet_ice_ctx_start(m_pIceCtxVideo)) != 0)
		{
			GM_DEBUG_WARN("tnet_ice_ctx_start() failed with error code = %d", iRet);
			GM_CHECK_HR(hr = E_FAIL);
		}
	}
	if ((m_eMediaType & GmMediaType_ScreenCast) == GmMediaType_ScreenCast)
	{
		if (m_pIceCtxScreenCast && (iRet = tnet_ice_ctx_start(m_pIceCtxScreenCast)) != 0)
		{
			GM_DEBUG_WARN("tnet_ice_ctx_start() failed with error code = %d", iRet);
			GM_CHECK_HR(hr = E_FAIL);
		}
	}

bail:
	return hr;
}


/*
* ICE callback function
* @param e New event.
* @retval <b>0</b> if no error; otherwise <b>error code</b>.
*/
int GmSessionCall::IceCallback(const struct tnet_ice_event_s *e)
{
	int ret = 0;
	GmSessionCall *This;

	This = (GmSessionCall *)e->userdata;

	GmAutoLock<GmSessionCall> autoLock(This);

	GM_DEBUG_INFO("ICE callback: %s", e->phrase);

	switch (e->type) {
	case tnet_ice_event_type_started: {
		// This->mIceState = IceStateGathering;
		break;
	}

	case tnet_ice_event_type_gathering_completed:
	case tnet_ice_event_type_conncheck_succeed:
	case tnet_ice_event_type_conncheck_failed:
	case tnet_ice_event_type_cancelled: {
		if (e->type == tnet_ice_event_type_gathering_completed)
		{
			if (This->IceGotLocalCandidatesAll())
			{
				GM_DEBUG_INFO("!!! ICE gathering done !!!");
				// This->sendSdp();
			}
		}
		else if (e->type == tnet_ice_event_type_conncheck_succeed)
		{
			if (This->IceIsDone())
			{
				if (This->m_bStartDeferred)
				{
					This->Start();
					This->m_bStartDeferred = false;
				}
			}
		}
		else if (e->type == tnet_ice_event_type_conncheck_failed || e->type == tnet_ice_event_type_cancelled)
		{
		}
		break;
	}

										// fatal errors which discard ICE process
	case tnet_ice_event_type_gathering_host_candidates_failed:
	case tnet_ice_event_type_gathering_reflexive_candidates_failed: {
		break;
	}
	}

	return ret;
}
