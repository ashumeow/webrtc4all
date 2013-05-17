/*
* Copyright (C) 2013 Gotham City. All rights reserved.
*/
/**@file dllmain.cc
 * @brief Implementation for DLL exports and COM class factory for registration. 
 * If 'Regsvr32.exe' works it's maily because of this.
 *
 * @author Batman@GothamCity
 */
#ifndef _GOTHAM_TEST_RTP_H
#define _GOTHAM_TEST_RTP_H

#define GOTHAM_TEST_CLIENT			1 /* 0: client, any other value: server */

#define GOTHAM_IP_LOCAL				"0.0.0.0"
#define GOTHAM_IP_REMOTE			"192.168.0.3"
#define GOTHAM_PORT_REMOTE_AUDIO	"1600"
#define GOTHAM_PORT_REMOTE_VIDEO	"1601"

#define SDP_RO \
        "v=0\r\n" \
        "o=batman 2890844526 2890844526 IN IP4 host.atlanta.example.com\r\n" \
        "s=\r\n" \
        "i=Gotham City\r\n" \
        "e=Batman@GothamCity (Batman)\r\n" \
        "c=IN IP4 " GOTHAM_IP_REMOTE "\r\n" \
        "t=3034423619 3042462419\r\n" \
        "m=audio " GOTHAM_PORT_REMOTE_AUDIO " RTP/AVP 0 8\r\n" \
        "c=IN IP4 " GOTHAM_IP_REMOTE "\r\n" \
        "a=rtpmap:8 PCMA/8000\r\n" \
        "a=rtpmap:0 PCMU/8000\r\n" \
        "m=video " GOTHAM_PORT_REMOTE_VIDEO " RTP/AVP 98\r\n" \
        "i=Video line\r\n" \
        "a=rtpmap:98 H264/90000\r\n" \
		"a=fmtp:98 profile-level-id=42e01e; packetization-mode=1; max-br=452; max-mbps=11880\r\n" \

void test_rtp_client()
{
    tmedia_session_mgr_t* mgr;
    const tsdp_message_t* sdp_lo;
    tsdp_message_t* sdp_ro;
    char* temp;
    tmedia_type_t type = (tmedia_type_t)(tmedia_video | tmedia_audio);

    mgr = tmedia_session_mgr_create(type,
            GOTHAM_IP_LOCAL, tsk_false, tsk_true/* offerer */);

    /* get lo (add to outgoing INVITE) */
    sdp_lo = tmedia_session_mgr_get_lo(mgr);
    if((temp = tsdp_message_tostring(sdp_lo))){
        TSK_DEBUG_INFO("sdp_lo=%s", temp);
        TSK_FREE(temp);
    }

    /* set ro (get from incoming 200 OK)*/
    if((sdp_ro = tsdp_message_parse(SDP_RO, tsk_strlen(SDP_RO)))){
        tmedia_session_mgr_set_ro(mgr, sdp_ro, tmedia_ro_type_answer);
        TSK_OBJECT_SAFE_FREE(sdp_ro);
    }

    /* start() */
    tmedia_session_mgr_start(mgr);

    getchar();

    TSK_OBJECT_SAFE_FREE(mgr);
}

void test_rtp_server()
{
    tmedia_session_mgr_t* mgr;
    const tsdp_message_t* sdp_lo;
    tsdp_message_t* sdp_ro;
    char* temp;
    tmedia_type_t type;

    /* get ro (from incoming INVITE) */
    if((sdp_ro = tsdp_message_parse(SDP_RO, tsk_strlen(SDP_RO)))){
        type = tmedia_video;
        mgr = tmedia_session_mgr_create(type,
                GOTHAM_IP_LOCAL, tsk_false, tsk_false/* answerer */);
        tmedia_session_mgr_set_ro(mgr, sdp_ro, tmedia_ro_type_offer);
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

    /* ACK */

    /* start() */
    tmedia_session_mgr_start(mgr);

    getchar();

    TSK_OBJECT_SAFE_FREE(mgr);
}

void test_rtp()
{   
#if GOTHAM_TEST_CLIENT
    test_rtp_client();
#else
    test_rtp_server();
#endif
}

#endif /* _GOTHAM_TEST_RTP_H */
