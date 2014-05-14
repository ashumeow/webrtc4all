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
#include "_Utils.h"
#include "_PeerConnection.h"
#include "_NetTransport.h"

#include <Shlwapi.h>

BOOL g_bInitialized = FALSE;
BOOL g_bHasDebugConsole = FALSE;
char* g_sNullTerminated = NULL;
UINT g_iNullTerminated = 0;
INT g_nEchoTail = 100;
CRITICAL_SECTION g_CS;

_Utils::_Utils()
{
	
}

_Utils::~_Utils()
{
	
}

void _Utils::Initialize(void)
{
	if(!g_bInitialized){

#if 0
		StartDebug();
#endif

		int iRet;
		if((iRet = tnet_startup()) != 0){
			TSK_DEBUG_ERROR("tnet_startup failed with error code=%d", iRet);
			return;
		}
		if(tdav_init() == 0){
			g_bInitialized = TRUE;
			TSK_DEBUG_INFO("Library succeesfully initilized");
		}
		else{
			TSK_DEBUG_ERROR("Failed to initialize");
		}

		InitializeCriticalSection(&g_CS);
		
#if METROPOLIS /* G2J.COM TelePresence client */
		static tmedia_srtp_mode_t g_eSrtpMode = tmedia_srtp_mode_optional; // "TANDBERG/4120 (X7.2.2.2) returns SAVPF without crypto lines"
		static tmedia_srtp_type_t g_eSrtpType = tmedia_srtp_type_sdes;
		static tsk_bool_t g_bIceEnabled = tsk_false;
		static tsk_bool_t g_bZeroartifactsEnabled = tsk_true;
		static tmedia_mode_t g_eAvpfMode = tmedia_mode_optional;
		// See issue 382: https://code.google.com/p/doubango/issues/detail?id=382
		static int g_nMotionRank = 1; // low(1),medium(2),high(4)
#else
		static tmedia_srtp_mode_t g_eSrtpMode = tmedia_srtp_mode_optional;
		static tmedia_srtp_type_t g_eSrtpType = (tmedia_srtp_type_t)(tmedia_srtp_type_sdes | tmedia_srtp_type_dtls);
		static tsk_bool_t g_bIceEnabled = tsk_true;
		static tsk_bool_t g_bZeroartifactsEnabled = tsk_true;
		static tmedia_mode_t g_eAvpfMode = tmedia_mode_optional;
		static int g_nMotionRank = 2; // low(1),medium(2),high(4)
#endif

		// Disable AMR, G.729, H.261 codecs
		tdav_set_codecs((tdav_codec_id_t)(
				tdav_codec_id_gsm |
				tdav_codec_id_pcma |
				tdav_codec_id_pcmu |
				/*tdav_codec_id_opus |*/ // FIXME: direct call to chrome sometimes produce noise. Why?
				tdav_codec_id_ilbc |
				tdav_codec_id_speex_nb |
				tdav_codec_id_speex_wb |
				tdav_codec_id_speex_uwb |
				tdav_codec_id_g722 |
#if !METROPOLIS
				tdav_codec_id_h263 |
				tdav_codec_id_h263p |
				tdav_codec_id_h263pp |
#endif
				tdav_codec_id_h264_bp |
				tdav_codec_id_h264_mp |
				tdav_codec_id_h264_hp |
#if !METROPOLIS
				tdav_codec_id_theora |
				tdav_codec_id_mp4ves_es |
#endif
				tdav_codec_id_vp8
		));

		// Priority: VP8, H.264 (BP then MP), PCMA/PCMU, OPUS...
		int prio = 0;
		tdav_codec_set_priority(tdav_codec_id_vp8, prio++);
		tdav_codec_set_priority(tdav_codec_id_h264_bp, prio++);
		tdav_codec_set_priority(tdav_codec_id_h264_mp, prio++);
		tdav_codec_set_priority(tdav_codec_id_pcma, prio++);
		tdav_codec_set_priority(tdav_codec_id_pcmu, prio++);
		tdav_codec_set_priority(tdav_codec_id_opus, prio++);

#if 0 /* RTCWeb optional to allow interop with other SIP implementations */
		tmedia_defaults_set_profile(tmedia_profile_rtcweb);
#else
		tmedia_defaults_set_ice_enabled(g_bIceEnabled); // Use ICE only if supported by remote party
		tmedia_defaults_set_srtp_mode(g_eSrtpMode); // Use SRTP only if supported by remote party
		tmedia_defaults_set_srtp_type(g_eSrtpType); // Negotiate the type of SRTP to use (SDES or DTLS)
#endif
		tmedia_defaults_set_rtcp_enabled(tsk_true);
		tmedia_defaults_set_rtcpmux_enabled(tsk_true);

		tmedia_defaults_set_avpf_mode(g_eAvpfMode);

		tmedia_defaults_set_echo_supp_enabled(tsk_true);
		tmedia_defaults_set_echo_tail(g_nEchoTail);
		tmedia_defaults_set_echo_skew(0);
		tmedia_defaults_set_agc_enabled(tsk_true);
		tmedia_defaults_set_vad_enabled(tsk_false);
		tmedia_defaults_set_noise_supp_enabled(tsk_true);
		tmedia_defaults_set_jb_margin(0);
		tmedia_defaults_set_jb_max_late_rate(1);

		tmedia_defaults_set_video_fps(15);
		tmedia_defaults_set_video_zeroartifacts_enabled(g_bZeroartifactsEnabled);
		tmedia_defaults_set_pref_video_size(tmedia_pref_video_size_vga);
		
		tmedia_defaults_set_opus_maxcapturerate(16000); /* Because of WebRTC AEC only 8000 and 16000 are supported */
		tmedia_defaults_set_opus_maxplaybackrate(48000);

		tmedia_defaults_set_video_motion_rank(g_nMotionRank);
		tmedia_defaults_set_bandwidth_video_download_max(-1);
		tmedia_defaults_set_bandwidth_video_upload_max(-1);
	}
}

bool _Utils::StartDebug(void)
{
	if (AllocConsole()){
		freopen("CONIN$", "r", stdin); 
		freopen("CONOUT$", "w", stdout); 
		freopen("CONOUT$", "w", stderr);
		SetConsoleTitleA("WebRTC extension for Safari, Opera, FireFox and IE");
		return TRUE;
	}
	return FALSE;
}

bool _Utils::StopDebug(void)
{
	return (FreeConsole() == TRUE);
}

static const HMODULE GetCurrentModule()
{
	HMODULE hm = {0};
    GetModuleHandleEx(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS, (LPCTSTR)GetCurrentModule, &hm);   
    return hm;
}

const char* _Utils::GetCurrentDirectoryPath()
{
	static char CURRENT_DIR_PATH[MAX_PATH] = { 0 };
	static DWORD CURRENT_DIR_PATH_LEN = 0;
	if(CURRENT_DIR_PATH_LEN == 0) {
		// NULL HMODULE will get the path to the executable not the DLL. When runing the code in Internet Explorer this is a BIG issue as the path is where IE.exe is installed.
		if((CURRENT_DIR_PATH_LEN = GetModuleFileNameA(GetCurrentModule(), CURRENT_DIR_PATH, MAX_PATH))) {
			if(!PathRemoveFileSpecA(CURRENT_DIR_PATH)) {
				TSK_DEBUG_ERROR("PathRemoveFileSpecA(%s) failed: %x", CURRENT_DIR_PATH, GetLastError());
				memset(CURRENT_DIR_PATH, 0, MAX_PATH);
			}
		}
		else {
			TSK_DEBUG_ERROR("GetModuleFileNameA() failed: %x", GetLastError());
		}
	}
	return CURRENT_DIR_PATH;
}


LRESULT CALLBACK _Utils::WndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch(uMsg){
		case WM_ICE_EVENT_CANDIDATE:
			{
				TSK_DEBUG_INFO("_Utils::WndProc::WM_ICE_EVENT");
				_PeerConnection* This = reinterpret_cast<_PeerConnection*>(wParam);
				PeerConnectionEvent* oEvent = reinterpret_cast<PeerConnectionEvent*>(lParam);
				if(oEvent){
					This->IceCallbackFire(oEvent);
					delete oEvent;
				}
				break;
			}
		case WM_ICE_EVENT_CONNECTED: /* Both parties support ICE and ConnCheck succeed */
		case WM_ICE_EVENT_FAILED: /* Both parties support ICE but ConnCheck failed: Use Address in SDP "c=" line */
		case WM_ICE_EVENT_CANCELLED: /* Remote paty doesn't support: Use Address in SDP "c=" line */
			{
				const char* pcMsg = uMsg == WM_ICE_EVENT_CONNECTED ? "WM_ICE_EVENT_CONNECTED"
						: uMsg == WM_ICE_EVENT_FAILED ? "WM_ICE_EVENT_FAILED"
						: uMsg == WM_ICE_EVENT_CANCELLED ? "WM_ICE_EVENT_CANCELLED"
						: "WM_ICE_EVENT_UNKNOWN";
				TSK_DEBUG_INFO("_Utils::WndProc::%s", pcMsg);
				_PeerConnection* This = reinterpret_cast<_PeerConnection*>(wParam);
				This->StartMedia(); // Start() must not be called on worker thread
				break;
			}
		case WM_NET_EVENT:
			{
				TSK_DEBUG_INFO("_Utils::WndProc::WM_NET_EVENT");		
				_NetTransport* This = reinterpret_cast<_NetTransport*>(wParam);
				NetTransportEvent* oEvent = reinterpret_cast<NetTransportEvent*>(lParam);
				if(oEvent){
					This->DgramCbFire(oEvent);
					delete oEvent;
				}
				break;
			}

		case WM_RFC5168_EVENT:
			{
				TSK_DEBUG_INFO("_Utils::WndProc::WM_RFC5168_EVENT");
				_PeerConnection* This = reinterpret_cast<_PeerConnection*>(wParam);
				const char* sCommand = reinterpret_cast<const char*>(lParam);
				if (sCommand) {
					This->Rfc5168CallbackFire(sCommand);
					// must not free (it's a static string)
				}
				break;
			}
	}
	return DefWindowProc(hWnd, uMsg, wParam, lParam);
}