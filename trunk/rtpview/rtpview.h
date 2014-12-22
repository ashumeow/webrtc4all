#pragma once

#define LOOPBACK_IP4                "127.0.0.1"
#define RTPVIEW_INI_FILE            "rtpview.ini"
#define RTPVIEW_MEDIA_TYPE			(tmedia_type_t)(tmedia_video | tmedia_audio)

static int attach_video_displays(tmedia_session_mgr_t* mgr)
{
    _ASSERT(mgr);
    int ret = 0;
    static const tmedia_type_t __tmedia_video = tmedia_video;
    if((RTPVIEW_MEDIA_TYPE & tmedia_video) == tmedia_video){
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

#define OCTET_EXACT_MATCH_DEPTH 8

inline int GetOctetMsbMatchDepth(int aOct, int bOct)
{
    if        ((aOct & 0xFF) == (bOct & 0xFF)) {
        return OCTET_EXACT_MATCH_DEPTH;
    } else if ((aOct & 0xFE) == (bOct & 0xFE)) {
        return OCTET_EXACT_MATCH_DEPTH-1;
    } else if ((aOct & 0xFC) == (bOct & 0xFC)) {
        return OCTET_EXACT_MATCH_DEPTH-2;
    } else if ((aOct & 0xF8) == (bOct & 0xF8)) {
        return OCTET_EXACT_MATCH_DEPTH-3;
    } else if ((aOct & 0xF0) == (bOct & 0xF0)) {
        return OCTET_EXACT_MATCH_DEPTH-4;
    } else if ((aOct & 0xE0) == (bOct & 0xE0)) {
        return OCTET_EXACT_MATCH_DEPTH-5;
    } else if ((aOct & 0xC0) == (bOct & 0xC0)) {
        return OCTET_EXACT_MATCH_DEPTH-6;
    } else if ((aOct & 0x80) == (bOct & 0x80)) {
        return OCTET_EXACT_MATCH_DEPTH-7;
    } else {
        return 0;
    }
}
