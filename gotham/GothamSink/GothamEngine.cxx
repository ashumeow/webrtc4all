#include "tinynet.h"

#include "tsk_debug.h"

#include "tinydav/tdav.h"

#include "tinymedia.h"

#include "GothamProducers.h"
#include "GothamEngine.h"
#include "GothamSinkUtils.h"
#include "GothamConsumers.h"

#include <assert.h>

bool GmEngine::s_bInitialized = false;

#define DEFAULT_CODECS							(tmedia_codec_id_h264_mp | tmedia_codec_id_h264_bp | tmedia_codec_id_vp8 | tmedia_codec_id_pcma | tmedia_codec_id_pcmu | tmedia_codec_id_opus) // use "tmedia_codec_id_all" to enable all codecs

#define DEFAULT_VIDEO_SIZE						tmedia_pref_video_size_vga
#define DEFAULT_VIDEO_FPS						25 // up to 120
#define DEFAULT_VIDEO_BANDWIDTH_UP				-1 // Kbps
#define DEFAULT_VIDEO_BANDWIDTH_DOWN			-1 // Kbps
#define DEFAULT_VIDEO_MR						1 // Motion rank (1, 2 or 4)
#define DEFAULT_VIDEO_ZERO_ARTIFACTS_ENABLED	tsk_true

#define DEFAULT_AUDIO_ECHO_SUPP_ENABLED			tsk_true
#define DEFAULT_AUDIO_ECHO_SKEW					0
#define DEFAULT_AUDIO_ECHO_TAIL					100

#define DEFAULT_RTCP_ENABLED					tsk_true
#define DEFAULT_RTCPMUX_ENABLED					tsk_true


#define DEFAULT_ICE_ENABLED						tsk_false

HRESULT GmEngine::Initialize()
{
	HRESULT hr = S_OK;

	if (!GmEngine::s_bInitialized)
	{
		if (tnet_startup() != 0)
		{
			GM_CHECK_HR(hr = E_FAIL);
		}

		if (tdav_init() != 0) 
		{
			GM_CHECK_HR(hr = E_FAIL);
		}

#if 0 // WebRTC features
		GM_ASSERT(tmedia_defaults_set_profile(tmedia_profile_rtcweb) == 0);
		GM_ASSERT(tmedia_defaults_set_avpf_mode(tmedia_mode_mandatory) == 0);
		GM_ASSERT(tmedia_defaults_set_srtp_type(tmedia_srtp_type_dtls) == 0);
		GM_ASSERT(tmedia_defaults_set_srtp_mode(tmedia_srtp_mode_mandatory) == 0);
#endif
		GM_ASSERT(tmedia_defaults_set_avpf_mode(tmedia_mode_mandatory) == 0);

		GM_ASSERT(tmedia_defaults_set_ice_enabled(DEFAULT_ICE_ENABLED) == 0);

		GM_ASSERT(tmedia_defaults_set_rtcp_enabled(DEFAULT_RTCP_ENABLED) == 0);
		GM_ASSERT(tmedia_defaults_set_rtcpmux_enabled(DEFAULT_RTCPMUX_ENABLED) == 0);

		GM_ASSERT(tmedia_defaults_set_pref_video_size(DEFAULT_VIDEO_SIZE) == 0);
		GM_ASSERT(tmedia_defaults_set_video_fps(DEFAULT_VIDEO_FPS) == 0);
		GM_ASSERT(tmedia_defaults_set_video_motion_rank(DEFAULT_VIDEO_MR) == 0);
		GM_ASSERT(tmedia_defaults_set_bandwidth_video_download_max(DEFAULT_VIDEO_BANDWIDTH_DOWN) == 0);
		GM_ASSERT(tmedia_defaults_set_bandwidth_video_upload_max(DEFAULT_VIDEO_BANDWIDTH_UP) == 0);

		GM_ASSERT(tmedia_defaults_set_echo_supp_enabled(DEFAULT_AUDIO_ECHO_SUPP_ENABLED) == 0);
		GM_ASSERT(tmedia_defaults_set_echo_skew(DEFAULT_AUDIO_ECHO_SKEW) == 0);
		GM_ASSERT(tmedia_defaults_set_echo_tail(DEFAULT_AUDIO_ECHO_TAIL) == 0);

		GM_ASSERT(tmedia_defaults_set_opus_maxcapturerate(48000) == 0);
		GM_ASSERT(tmedia_defaults_set_opus_maxplaybackrate(48000) == 0);

		GM_ASSERT(tdav_set_codecs((tdav_codec_id_t)DEFAULT_CODECS) == 0);
		GM_ASSERT(tdav_codec_set_priority((tdav_codec_id_t)tmedia_codec_id_h264_mp, 0) == 0);
		GM_ASSERT(tdav_codec_set_priority((tdav_codec_id_t)tmedia_codec_id_h264_bp, 1) == 0);
		GM_ASSERT(tdav_codec_set_priority((tdav_codec_id_t)tmedia_codec_id_vp8, 2) == 0);
		GM_ASSERT(tdav_codec_set_priority((tdav_codec_id_t)tmedia_codec_id_opus, 3) == 0);
		GM_ASSERT(tdav_codec_set_priority((tdav_codec_id_t)tmedia_codec_id_pcma, 4) == 0);
		GM_ASSERT(tdav_codec_set_priority((tdav_codec_id_t)tmedia_codec_id_opus, 5) == 0);

		// Unregister all video producers (no camera)
		GM_ASSERT(tmedia_producer_plugin_unregister_by_type(tmedia_video) == 0);
		// Unregister all video consumers (no display)
		GM_ASSERT(tmedia_consumer_plugin_unregister_by_type(tmedia_video) == 0);

		// register video consumer
		GM_ASSERT(tmedia_producer_plugin_register(gm_producer_plugin_def_t) == 0);
		// Register video passthrough "display"
		GM_ASSERT(tmedia_consumer_plugin_register(gm_display_plugin_def_t) == 0);

		// video camera friendly strings for testing
		// 0: "GEIT MViQ Probe Video"
		// 1: "Logitech Webcam C930e"
		// 2: "Integrated Camera - Lenovo ThinkPad"
		// 3: "Rocketfish HD Webcam Pro"
		// 4: "FaceTime HD Camera (Built-in) - iMAC 2003"
		// 5: "Logitech HD Pro Webcam C920"
		// If none is connected then the default device will be selected
		GM_ASSERT(tmedia_producer_set_friendly_name(tmedia_video, "Logitech HD Pro Webcam C920") == 0);

		GmEngine::s_bInitialized = true;
	}

bail:
	return hr;
}

HRESULT GmEngine::DeInitialize()
{
	if (GmEngine::s_bInitialized)
	{
		tdav_deinit();
		tnet_cleanup();

		GmEngine::s_bInitialized = false;
	}

	return S_OK;
}