/* Copyright (C) 2013 Mamadou DIOP
* Copyright (C) 2013 Doubango Telecom <http://www.doubango.org>
*	
* This file is part of Open Source Doubango Framework.
*
* DOUBANGO is free software: you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation, either version 3 of the License, or
* (at your option) any later version.
*	
* DOUBANGO is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*	
* You should have received a copy of the GNU General Public License
* along with DOUBANGO.
*/
#ifndef _GOTHAM_TEST_RTP_H
#define _GOTHAM_TEST_RTP_H

#include <assert.h>

/* whether to perform local testing
0: send encoded RTP to remove PC with IP address and port defined below (GOTHAM_IP_REMOTE, GOTHAM_PORT_REMOTE_AUDIO and GOTHAM_PORT_REMOTE_VIDEO)
Otherwise: send encoded RTP to ourself for decoding
*/
#if !defined(GOTHAM_TEST_LOOPBACK)
#	define GOTHAM_TEST_LOOPBACK		1 
#endif

#define GOTHAM_MEDIA_TYPE			(tmedia_type_t)(tmedia_video | tmedia_audio)

#define GOTHAM_IP_LOCAL				"0.0.0.0"
#define GOTHAM_IP_REMOTE			"192.168.0.2"
#define GOTHAM_PORT_LOCAL_AUDIO		"1700"
#define GOTHAM_PORT_LOCAL_VIDEO		"1701"
#define GOTHAM_PORT_REMOTE_AUDIO	"1600"
#define GOTHAM_PORT_REMOTE_VIDEO	"1601"

#define SDP_BUILD(IP, PORT_AUDIO, PORT_VIDEO) \
		"v=0\r\n" \
        "o=batman 2890844526 2890844526 IN IP4 host.atlanta.example.com\r\n" \
        "s=\r\n" \
        "i=Gotham City (Remote SDP)\r\n" \
        "e=Batman@GothamCity (Batman)\r\n" \
        "c=IN IP4 " IP "\r\n" \
        "t=3034423619 3042462419\r\n" \
        "m=audio " PORT_AUDIO " RTP/AVP 0 8\r\n" \
        "c=IN IP4 " IP "\r\n" \
        "a=rtpmap:8 PCMA/8000\r\n" \
        "a=rtpmap:0 PCMU/8000\r\n" \
        "m=video " PORT_VIDEO " RTP/AVP 98\r\n" \
        "i=Video line\r\n" \
        "a=rtcp-fb:* nack pli" \
		"a=rtcp-fb:* ccm fir" \
		"a=rtcp-fb:* goog-remb" \
		"a=rtpmap:104 H264/90000" \
		"a=imageattr:104 recv [x=[128:16:640],y=[96:16:480]] send [x=[128:16:640],y=[96:16:480]]" \
		"a=fmtp:104 profile-level-id=42001e; packetization-mode=1; impl=FFMPEG" \
		"a=rtpmap:105 H264/90000" \
		"a=imageattr:105 recv [x=[128:16:640],y=[96:16:480]] send [x=[128:16:640],y=[96:16:480]]" \
		"a=fmtp:105 profile-level-id=4d001e; packetization-mode=1; impl=FFMPEG" \
		"a=rtpmap:103 H263-1998/90000" \
		"a=fmtp:103 CIF=2;QCIF=2;SQCIF=2" \
		"a=rtpmap:102 H263-2000/90000" \
		"a=fmtp:102 CIF=2;QCIF=2;SQCIF=2" \
		"a=rtpmap:125 theora/90000" \
		"a=imageattr:125 recv [x=[128:16:640],y=[96:16:480]] send [x=[128:16:640],y=[96:16:480]]" \
		"a=fmtp:125 sampling=YCbCr-4:2:0; width=640; height=480" \
		"a=rtpmap:34 H263/90000" \
		"a=fmtp:34 CIF=2;QCIF=2;SQCIF=2" \
		"a=rtpmap:31 H261/90000" \
		"a=imageattr:31 QCIF=2" \
		"a=fmtp:31 QCIF=2" \
		"a=pcfg:1 t=1" \
		"a=sendrecv" \
		"a=rtcp-mux" \
		"a=ssrc:2654140548 cname:8c5af292a4eed56688ce3b86e3ce813b" \
		"a=ssrc:2654140548 mslabel:6994f7d1-6ce9-4fbd-acfd-84e5131ca2e2" \
		"a=ssrc:2654140548 label:doubango@video"

#define SDP_RO \
	SDP_BUILD(GOTHAM_IP_REMOTE, GOTHAM_PORT_REMOTE_AUDIO, GOTHAM_PORT_REMOTE_VIDEO)
#define SDP_LO \
	SDP_BUILD(GOTHAM_IP_LOCAL, GOTHAM_PORT_LOCAL_AUDIO, GOTHAM_PORT_LOCAL_VIDEO)

static int attach_video_displays(tmedia_session_mgr_t* mgr)
{
	assert(mgr);
	int ret = 0;
	static const tmedia_type_t __tmedia_video = tmedia_video;
	if((GOTHAM_MEDIA_TYPE & tmedia_video) == tmedia_video){
#if TMEDIA_UNDER_WINDOWS
		INT64 nWindowRemote = reinterpret_cast<INT64>(GetRemoteHwnd());
		INT64 nWindowLocal = reinterpret_cast<INT64>(GetLocalHwnd());
		ret = tmedia_session_mgr_set(mgr,
			TMEDIA_SESSION_CONSUMER_SET_INT64(__tmedia_video, "remote-hwnd", nWindowRemote),
			TMEDIA_SESSION_SET_NULL());
		if(ret == 0){
			ret = tmedia_session_mgr_set(mgr,
				TMEDIA_SESSION_PRODUCER_SET_INT64(__tmedia_video, "local-hwnd", nWindowLocal),
				TMEDIA_SESSION_SET_NULL());
		}
#endif
	}
	return ret;
}

static void test_rtp_client()
{
    tmedia_session_mgr_t* mgr;
    const tsdp_message_t* sdp_lo;
    tsdp_message_t* sdp_ro;
    char* temp;
	int ret = 0;
    tmedia_type_t type = GOTHAM_MEDIA_TYPE;

    mgr = tmedia_session_mgr_create(type,
            GOTHAM_IP_LOCAL, tsk_false, tsk_true/* offerer */);
	assert(mgr);

    /* get lo (add to outgoing INVITE) */
    sdp_lo = tmedia_session_mgr_get_lo(mgr);
    if((temp = tsdp_message_tostring(sdp_lo))){
        TSK_DEBUG_INFO("sdp_lo=%s", temp);
        TSK_FREE(temp);
    }

    /* set ro (get from incoming 200 OK)*/
#if GOTHAM_TEST_LOOPBACK
	(void)(sdp_ro);
	assert((ret = tmedia_session_mgr_set_ro(mgr, sdp_lo, tmedia_ro_type_answer)) == 0);
#else
    if((sdp_ro = tsdp_message_parse(SDP_RO, tsk_strlen(SDP_RO)))){
        assert((ret = tmedia_session_mgr_set_ro(mgr, sdp_ro, tmedia_ro_type_answer)) == 0);
        TSK_OBJECT_SAFE_FREE(sdp_ro);
    }
#endif

	/* attach video dislays */
	assert((ret = attach_video_displays(mgr)) == 0);

    /* start() */
    assert((ret = tmedia_session_mgr_start(mgr)) == 0);

    getchar();

    TSK_OBJECT_SAFE_FREE(mgr);
}

static void test_rtp_server()
{
    tmedia_session_mgr_t* mgr;
    const tsdp_message_t* sdp_lo;
    tsdp_message_t* sdp_ro;
    char* temp;
	int ret = 0;
#if GOTHAM_TEST_LOOPBACK
	static const char* __sdp_ro = SDP_LO;
#else
	static const char* __sdp_ro = SDP_RO;
#endif

    /* get ro (from incoming INVITE) */
    if((sdp_ro = tsdp_message_parse(__sdp_ro, tsk_strlen(__sdp_ro)))){
        mgr = tmedia_session_mgr_create(GOTHAM_MEDIA_TYPE,
                GOTHAM_IP_LOCAL, tsk_false, tsk_false/* answerer */);
        assert((ret = tmedia_session_mgr_set_ro(mgr, sdp_ro, tmedia_ro_type_offer)) == 0);
        TSK_OBJECT_SAFE_FREE(sdp_ro);
    }
    else{
        TSK_DEBUG_ERROR("Failed to deserialize remote sdp");
        return;
    }


    /* get lo (add to outgoing 200 OK) */
    sdp_lo = tmedia_session_mgr_get_lo(mgr);
    if((temp = tsdp_message_tostring(sdp_lo))){
        TSK_DEBUG_INFO("sdp_lo=%s", temp);
        TSK_FREE(temp);
    }

    /* attach video dislays */
	assert((ret = attach_video_displays(mgr)) == 0);

    /* start() */
    assert((ret = tmedia_session_mgr_start(mgr)) == 0);

    getchar();

    TSK_OBJECT_SAFE_FREE(mgr);
}

void test_rtp(int run_as_client)
{   
	if(run_as_client) {
		test_rtp_client();
	}
	else {
		test_rtp_server();
	}
}

#endif /* _GOTHAM_TEST_RTP_H */
